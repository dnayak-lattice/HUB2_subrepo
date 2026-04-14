"""
============================================================================
Katana Visualizer — Flask Backend Application
============================================================================

This module implements the server-side component of the Katana Visualizer
web application. It exposes a set of REST API endpoints consumed by the
browser-based HMI and manages all hardware interactions through the
CamInterface and IPC layers.

Core Responsibilities
---------------------
1. Camera Management
   - Initialises CLNX (USB Camera) via CamInterface.
   - Provides start / stop control for each camera through a REST endpoint.
   - Serves live MJPEG video streams with real-time bounding-box overlays.

2. AI Pipeline Control (via IPC)
   - Start / stop the SOM video-processing pipeline on the C backend.
   - Enable / disable face-identification inference at runtime.
   - Communicate with the C backend through the AppIPCManager IPC client.

3. Face Identification
   - Generate new face IDs when a person is in the ideal pose.
   - Register (save) face profiles with user-supplied names.
   - List and delete enrolled face profiles stored in face_ids.json.

4. Bounding-Box Overlay Engine
   - Receives per-face detection events from the C backend via an IPC
     callback and accumulates them into an atomic per-frame snapshot.
   - A dedicated plotter thread composites bounding boxes, identity labels,
     PPE badges, and icon overlays onto the raw camera frames at ~20 FPS.
   - Stale detections are automatically cleared when the backend stops
     sending events (configurable timeout).

5. Configuration
   - Loads settings from config.json (camera resolution, server host/port,
     overlay colours, thickness, icon paths, etc.).
   - Serves a subset of the configuration to the frontend via /api/config.

6. Graceful Shutdown
   - Handles SIGINT / SIGTERM and KeyboardInterrupt to cleanly stop
     background threads and release camera hardware before exit.

Dependencies
------------
- Flask           — HTTP server and routing
- OpenCV (cv2)    — Frame decoding, overlay drawing, JPEG encoding
- NumPy           — Image array manipulation
- CamInterface    — Abstraction over Pi Camera 2 and USB (V4L2) cameras
- ipc_client      — IPC bridge to the Lattice C backend pipeline

API Endpoints
-------------
GET  /                          Serve the main HMI page
GET  /api/config                Frontend configuration subset
GET  /api/live_detection        Latest primary-face detection flags
GET  /video_feed/cpnx           MJPEG stream — CPNX (Pi Camera)
GET  /video_feed/clnx           MJPEG stream — CLNX (USB Camera)
POST /camera_control            Start or stop a camera
GET  /camera_status/<type>      Query camera running state
POST /api/start_pipeline        Start the SOM video pipeline
POST /api/stop_pipeline         Stop the SOM video pipeline
POST /api/start_face_id         Enable face identification
POST /api/stop_face_id          Disable face identification
POST /api/generate_face_id      Generate a new face ID
POST /api/update_face_id        Save / update a face profile
GET  /api/face_profiles         List all enrolled face profiles
DELETE /api/face_profiles/<id>  Delete an enrolled face profile

============================================================================
"""

import os
import sys
import json
import time
import signal
import atexit
import traceback
import threading
from flask import Flask, render_template, jsonify, Response, request
import cv2
import numpy as np
from camInterface import CamInterface
import ipc_client as app_ipc

app = Flask(__name__)

ipc = app_ipc.AppIPCManager()
ipc.list_face_profiles()

def load_config():
    """
    Load configuration from config.json file.
    
    This function attempts to load the configuration file from the same directory
    as this script. It first tries to read a default 'config.json' file to get
    the actual config filename from the 'files.config_filename' field, then loads
    that file. If the file is not found or contains invalid JSON, it returns an
    empty dictionary and prints appropriate error messages. The function handles
    both FileNotFoundError and JSONDecodeError exceptions gracefully.
    
    Returns:
        dict: Configuration dictionary, or empty dict if loading fails
    """
    config_filename = 'config.json'
    try:
        temp_config_path = os.path.join(os.path.dirname(__file__), config_filename)
        with open(temp_config_path, 'r') as f:
            temp_config = json.load(f)
            config_filename = temp_config.get('files', {}).get('config_filename', config_filename)
    except:
        pass
    config_path = os.path.join(os.path.dirname(__file__), config_filename)
    try:
        with open(config_path, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        error_msg = CONFIG.get('errors', {}).get('config_not_found', 'Warning: config.json not found at {path}, using defaults').format(path=config_path)
        print(error_msg)
        return {}
    except json.JSONDecodeError as e:
        error_msg = CONFIG.get('errors', {}).get('config_parse_error', 'Error parsing config.json: {error}, using defaults').format(error=e)
        print(error_msg)
        return {}

CONFIG = load_config()


def _parse_bgr_triplet(item):
    """Return (B,G,R) uint8 tuple from a 3-element list, or None."""
    if isinstance(item, (list, tuple)) and len(item) == 3:
        try:
            return (int(item[0]) & 255, int(item[1]) & 255, int(item[2]) & 255)
        except (TypeError, ValueError):
            pass
    return None


def _load_face_overlay_settings(cfg: dict):
    """
    Bbox/PPE overlay: ideal uses ideal_bgr only; non-ideal uses non_ideal_palette_bgr
    with stable per-identity colors (see _bbox_colors_for_frame). OpenCV BGR.
    """
    fo = cfg.get("face_overlay") or {}
    default_non_ideal = [
        (255, 255, 0),
        (0, 165, 255),
        (203, 192, 255),
        (230, 180, 80),
        (255, 0, 0),
        (0, 255, 255),
        (255, 0, 255),
        (0, 255, 128),
        (128, 0, 255),
        (42, 180, 255),
    ]
    default_ppe = [
        (200, 255, 210),
        (150, 210, 255),
        (200, 192, 255),
    ]
    ideal = _parse_bgr_triplet(fo.get("ideal_bgr")) or (0, 255, 0)
    raw_palette = fo.get("non_ideal_palette_bgr")
    non_ideal = []
    if isinstance(raw_palette, list):
        for p in raw_palette:
            t = _parse_bgr_triplet(p)
            if t is not None:
                non_ideal.append(t)
    if not non_ideal:
        non_ideal = list(default_non_ideal)
    bt = fo.get("box_thickness_px") or {}
    thick_not = int(bt.get("not_ideal", 3))
    thick_ideal = int(bt.get("ideal", 20))
    ppe_raw = fo.get("ppe_badge_bgr")
    ppe_bg = []
    if isinstance(ppe_raw, list):
        for p in ppe_raw[:3]:
            t = _parse_bgr_triplet(p)
            if t is not None:
                ppe_bg.append(t)
    while len(ppe_bg) < 3:
        ppe_bg.append(default_ppe[len(ppe_bg)])
    return ideal, non_ideal, thick_not, thick_ideal, ppe_bg


_IDEAL_BGR, _NON_IDEAL_PALETTE, _BOX_THICK_NOT_IDEAL, _BOX_THICK_IDEAL, _PPE_BADGE_BG = _load_face_overlay_settings(CONFIG)

USB_CAMERA_ID = CONFIG.get('cameras', {}).get('usb', {}).get('id', 0)

try:
    cam_instance = CamInterface()

    try:
        usb_camera_config = CONFIG.get('cameras', {}).get('usb', {})
        cam_instance.setup_camera(
            USB_CAMERA_ID, 
            width=usb_camera_config.get('width', 1920), 
            height=usb_camera_config.get('height', 1080), 
            camera_type=usb_camera_config.get('type', 'usb')
        )
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('usb_camera_setup_failed', 'USB Camera setup failed: {error}').format(error=e)
        print(error_msg)
        traceback.print_exc()

except Exception as e:
    error_msg = CONFIG.get('errors', {}).get('hub_init_failed', 'Camera initialization failed: {error}').format(error=e)
    print(error_msg)
    traceback.print_exc()

shutdown_in_progress = False



_bbox_lock = threading.Lock()

# Non-ideal bbox colors: stable per person (enrolled face_id), spatial bucket for face_id==0.
_palette_assign_lock = threading.Lock()
_bbox_palette_key_to_index: dict[tuple, int] = {}

_BBOX_GEN_INTERVAL  = 1.0

# List of per-face bbox dicts for the current frame (empty = no detections)
_bbox_state = []

# Accumulation buffer – collects faces as IPC events arrive mid-frame
_bbox_accumulator = []




_bbox_gen_stop  = threading.Event()

# Timestamp of the last IPC event received (for stale-bbox clearing)
_last_event_time = 0.0
_BBOX_STALE_TIMEOUT = 1.0   # seconds with no event before bbox is cleared


def ipc_event_callback(event_data, face_count, face_index):
    """
    Called by the IPC event-listener thread for every detected face in a frame.
    Accumulates per-face data into _bbox_accumulator; on the last face of the
    frame (face_index == face_count) atomically replaces _bbox_state with the
    full list so the plotter always sees a consistent snapshot.

    Event tuple layout (from app_ipc regex):
        face_id, left, top, right, bottom, hat, glasses, gloves, ideal
    Icons list order: [glasses(0/1), gloves(0/1), hat(0/1)]
    """
    global _bbox_state, _bbox_accumulator, _last_event_time

    face_id, left, top, right, bottom, hat, glasses, gloves, ideal = event_data

    # Empty frame from C backend — clear boxes immediately, no stale lag
    if face_count == 0:
        print("[BBOX_CB] Empty frame — no faces detected, bboxes cleared")
        with _bbox_lock:
            _bbox_state = []
            _bbox_accumulator = []
        return

    # Log first face of every new frame so frame boundaries are visible in terminal
    if face_index == 1:
        print(f"[BBOX_CB] New frame received — {face_count} face(s) from C backend")

    x1 = int(left)
    y1 = int(top)
    x2 = int(right)
    y2 = int(bottom)

    # icons[0]=glasses, icons[1]=gloves, icons[2]=hat
    icons = [bool(glasses), bool(gloves), bool(hat)]

    label = "Unidentified Person"
    try:
        json_db_path = os.path.join(os.path.dirname(__file__), 'face_ids.json')
        if os.path.exists(json_db_path):
            with open(json_db_path, 'r') as f:
                profiles = json.load(f)
            for profile in profiles:
                if profile.get('face_id') == int(face_id):
                    label = profile.get('name', label)
                    break
    except Exception as e:
        print(f"[BBOX_CB] WARNING: face profile lookup failed for face_id={face_id}: {e}")

    print(
        f"[BBOX_CB]   face[{face_index}/{face_count}] "
        f"id={face_id}  label='{label}'  "
        f"bbox=[{x1},{y1},{x2},{y2}]  "
        f"ppe=[hat={int(hat)} glasses={int(glasses)} gloves={int(gloves)}]  "
        f"ideal={int(ideal)}"
    )

    face_entry = {
        "face_id": face_id,
        "ideal":   int(ideal),
        "label":   label,
        "x1":      x1,
        "y1":      y1,
        "x2":      x2,
        "y2":      y2,
        "icons":   icons,
    }
    print(face_entry)

    # First face of a new frame – start fresh accumulation
    if face_index == 1:
        _bbox_accumulator = [face_entry]
    else:
        _bbox_accumulator.append(face_entry)

    # Last face of the frame – commit the whole batch atomically
    if face_index == face_count:
        with _bbox_lock:
            _bbox_state = list(_bbox_accumulator)
        _last_event_time = time.monotonic()
        print(f"[BBOX_CB] Frame committed to overlay — {face_count} face(s) active")


ipc.start_event_listener(ipc_event_callback)


def _bbox_generator_worker():
    """
    Generator thread: clears the bounding-box state if no IPC event has
    arrived within _BBOX_STALE_TIMEOUT seconds.  This prevents the last
    detection being drawn forever when the C backend stops sending events.
    """
    while not _bbox_gen_stop.wait(timeout=_BBOX_GEN_INTERVAL):
        if _last_event_time and (time.monotonic() - _last_event_time) > _BBOX_STALE_TIMEOUT:
            with _bbox_lock:
                _bbox_state.clear()     # empty list = no bounding boxes


_FONT      = cv2.FONT_HERSHEY_SIMPLEX

# ── Pre-load icon images once at startup ─────────────────────────────────────
_ICON_SIZE = 90

_STATIC_DIR  = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'static'))

def _load_icon(filename: str, size: int) -> np.ndarray | None:
    """Load a PNG icon, resize to size×size, return BGRA ndarray or None."""
    path = os.path.join(_STATIC_DIR, filename)
    try:
        img = cv2.imread(path, cv2.IMREAD_UNCHANGED)
        if img is None:
            # fallback: try reading as bytes in case of unicode path issues
            with open(path, 'rb') as f:
                data = np.frombuffer(f.read(), dtype=np.uint8)
            img = cv2.imdecode(data, cv2.IMREAD_UNCHANGED)
        if img is None:
            print(f"[bbox] WARNING: could not load icon: {path}")
            return None
        return cv2.resize(img, (size, size), interpolation=cv2.INTER_AREA)
    except Exception as e:
        print(f"[bbox] WARNING: exception loading icon {path}: {e}")
        return None


_ICON_IMAGES = [
    _load_icon('icon1.png', _ICON_SIZE),
    _load_icon('icon2.png', _ICON_SIZE),
    _load_icon('icon3.png', _ICON_SIZE),
]
print(f"[bbox] Icon load results: {[img is not None for img in _ICON_IMAGES]} (static dir: {_STATIC_DIR})")


def _overlay_icon_black(dst: np.ndarray, icon: np.ndarray, x: int, y: int) -> None:
    """Composite PPE icon as pure black silhouette (high contrast on video)."""
    ih, iw = icon.shape[:2]
    x0, y0 = max(x, 0), max(y, 0)
    x1_src = x0 - x
    y1_src = y0 - y
    x1 = min(x + iw, dst.shape[1])
    y1 = min(y + ih, dst.shape[0])
    dw, dh = x1 - x0, y1 - y0
    if dw <= 0 or dh <= 0:
        return

    icon_crop = icon[y1_src:y1_src + dh, x1_src:x1_src + dw]
    dst_crop = dst[y0:y1, x0:x1]

    black = np.zeros((dh, dw, 3), dtype=np.float32)
    if icon_crop.shape[2] == 4:
        alpha = icon_crop[:, :, 3:4].astype(np.float32) / 255.0
    else:
        alpha = np.ones((dh, dw, 1), dtype=np.float32)
    blended = black * alpha + dst_crop.astype(np.float32) * (1.0 - alpha)
    dst[y0:y1, x0:x1] = np.clip(blended, 0, 255).astype(np.uint8)


def _draw_ppe_circle_badge(
    dst: np.ndarray,
    cx: int,
    iy: int,
    icon_src: np.ndarray,
    icon_sz: int,
    bg_bgr: tuple,
) -> int:
    """
    Draw a filled circle badge and centered black icon. Returns cell width (diameter + gap not included).
    """
    pad = 6
    diameter = int(icon_sz + 2 * pad)
    diameter = max(diameter, 24)
    r = diameter // 2
    cx_i = int(cx + r)
    cy_i = int(iy + r)
    cv2.circle(dst, (cx_i, cy_i), r, bg_bgr, -1, lineType=cv2.LINE_AA)
    cv2.circle(dst, (cx_i, cy_i), r, (80, 80, 80), 1, lineType=cv2.LINE_AA)

    ox = cx + (diameter - icon_sz) // 2
    oy = iy + (diameter - icon_sz) // 2
    small = cv2.resize(icon_src, (icon_sz, icon_sz), interpolation=cv2.INTER_AREA)
    _overlay_icon_black(dst, small, ox, oy)
    return diameter


def _bbox_palette_key(b: dict) -> tuple:
    """
    Stable key for non-ideal color assignment.
    Enrolled faces: ('id', face_id). Unidentified (face_id==0): quantized bbox top-left.
    """
    fid = int(b.get("face_id", 0))
    if fid != 0:
        return ("id", fid)
    q = 48
    x1 = int(b.get("x1", 0))
    y1 = int(b.get("y1", 0))
    return ("u", x1 // q, y1 // q)


def _first_free_palette_index(used: set[int], n: int, key: tuple) -> int:
    """Smallest index in 0..n-1 not in used; if all taken, deterministic hash fallback."""
    for j in range(n):
        if j not in used:
            return j
    return hash(key) % n


def _bbox_colors_for_frame(bboxes: list) -> list[tuple]:
    """
    One BGR color per bbox: ideal → ideal_bgr; non-ideal → colors from
    non_ideal_palette_bgr that are stable over time and, when possible, all
    different within the same frame (up to palette size, default 10).

    - Known face_id (non-zero): persistent palette index per face_id.
    - Unidentified (face_id==0): persistent key from quantized (x1,y1); two unknowns
      in the same cell may share a color; otherwise they get distinct slots when free.
    - If more than N identities compete for N palette slots, indices are reused
      (hash fallback); within a frame we still try to avoid duplicate colors.
    """
    palette = _NON_IDEAL_PALETTE
    n = len(palette)
    non_ideal_idx = [i for i, b in enumerate(bboxes) if not int(b.get("ideal", 0))]

    by_index: dict[int, tuple] = {}
    with _palette_assign_lock:
        used_this_frame: set[int] = set()
        for bi in non_ideal_idx:
            b = bboxes[bi]
            key = _bbox_palette_key(b)
            if key not in _bbox_palette_key_to_index:
                used_global = set(_bbox_palette_key_to_index.values())
                _bbox_palette_key_to_index[key] = _first_free_palette_index(used_global, n, key)
            pref = _bbox_palette_key_to_index[key]
            idx = pref
            if idx in used_this_frame:
                idx = _first_free_palette_index(used_this_frame, n, key)
                _bbox_palette_key_to_index[key] = idx
            used_this_frame.add(idx)
            by_index[bi] = tuple(palette[idx])

    out: list[tuple] = []
    for i, b in enumerate(bboxes):
        if int(b.get("ideal", 0)):
            out.append(tuple(_IDEAL_BGR))
        else:
            out.append(by_index[i])
    return out


def _draw_bboxes_on_frame(frame: np.ndarray, bboxes: list) -> np.ndarray:
    """Draw all detected face bounding boxes onto a copy of frame."""
    if not bboxes:
        return frame.copy()

    out = frame.copy()
    colors = _bbox_colors_for_frame(bboxes)
    for bbox, box_color in zip(bboxes, colors):
        _draw_single_bbox(out, bbox, box_color=box_color)
    return out


def _draw_single_bbox(out: np.ndarray, bbox: dict, box_color: tuple | None = None) -> None:
    """
    Draw one face bbox with a YOLO-style label tab and PPE badges.
    Label placement: above the box when there is room, otherwise flipped
    inside the box so it stays fully visible even at the top edge of the frame.
    """
    h, w = out.shape[:2]

    px1 = max(0, min(int(bbox["x1"]), w - 1))
    py1 = max(0, min(int(bbox["y1"]), h - 1))
    px2 = max(0, min(int(bbox["x2"]), w - 1))
    py2 = max(0, min(int(bbox["y2"]), h - 1))

    if box_color is None:
        bc = bbox.get("box_color")
        if bc is not None:
            box_color = tuple(int(x) for x in bc[:3])
        else:
            box_color = _bbox_colors_for_frame([bbox])[0]
    icons = bbox.get("icons", [True, True, True])

    name_text = bbox.get("label", "Unidentified Person")
    ideal_flag = int(bbox.get("ideal", 0))
    name_text = f"{name_text}  |  {'Ideal' if ideal_flag else 'Not ideal'}"

    name_fs = 1.32
    name_thick = 4
    pad_x = 10
    pad_y = 5
    badge_gap = 6

    box_thick = _BOX_THICK_IDEAL if ideal_flag else _BOX_THICK_NOT_IDEAL
    cv2.rectangle(out, (px1, py1), (px2, py2), box_color, box_thick)

    (nw, nh), n_base = cv2.getTextSize(name_text, _FONT, name_fs, name_thick)
    tab_h = nh + n_base + 2 * pad_y
    text_block_w = nw + 2 * pad_x
    tab_x2 = min(px1 + text_block_w, w - 1)


    label_inside = (py1 < tab_h * 0.4)

    txt_black = (0, 0, 0)

    if not label_inside:
        row_top = py1 - tab_h
        row_bot = py1
    else:
        row_top = py1
        row_bot = min(py1 + tab_h, py2, h - 1)

    cv2.rectangle(out, (px1, row_top), (tab_x2, row_bot), box_color, -1)
    actual_tab_h = row_bot - row_top
    name_ty = row_top + (actual_tab_h + nh) // 2
    cv2.putText(out, name_text, (px1 + pad_x, name_ty),
                _FONT, name_fs, txt_black, name_thick, cv2.LINE_AA)

    base_h = max(tab_h, 36)
    icon_sz = int(round(base_h * 1.5))
    icon_sz = min(icon_sz, 90)

    n_ppe = sum(
        1
        for i in range(min(3, len(icons)))
        if icons[i] and _ICON_IMAGES[i] is not None
    )
    if not n_ppe:
        return

    pad = 6
    cell_d = int(icon_sz + 2 * pad)
    strip_w = n_ppe * cell_d + max(0, n_ppe - 1) * badge_gap

    bw = max(1, px2 - px1)
    margin = 6
    tab_end = px1 + text_block_w + badge_gap
    mid_floor = px1 + int(0.38 * bw)

    cx_start = px2 - strip_w - margin
    cx_start = max(cx_start, tab_end, mid_floor)
    if cx_start + strip_w > px2 - margin:
        cx_start = max(tab_end, px2 - strip_w - margin)

    if not label_inside:
        iy = py1 - cell_d
        if iy < 0:
            iy = 0
    else:
        iy = row_top

    cx = float(cx_start)
    for i in range(min(3, len(icons))):
        if not icons[i] or _ICON_IMAGES[i] is None:
            continue
        bg = _PPE_BADGE_BG[i]
        step = _draw_ppe_circle_badge(out, int(cx), iy, _ICON_IMAGES[i], icon_sz, bg)
        cx += step + badge_gap


_bbox_generator_thread = threading.Thread(
    target=_bbox_generator_worker,
    name="BBoxGeneratorThread",
    daemon=True
)
_bbox_generator_thread.start()



def generate_frames_with_bbox(camera_id: int | str):
    """
    Consumes raw BGR numpy frames from the camera, draws bounding-box
    overlays inline, and performs a **single** JPEG encode before yielding
    the MJPEG chunk to the HTTP response.

    Previous pipeline (3 codec ops per frame):
        camera.read() → JPEG encode → yield → JPEG decode → overlay → JPEG encode
    Current pipeline (1 codec op per frame):
        camera.read() → overlay draw → JPEG encode → yield

    The plotter thread is no longer needed; bbox state is read directly
    from _bbox_state under the existing lock.
    """
    BOUNDARY_PREFIX = b'--frame\r\nContent-Type: image/jpeg\r\n\r\n'
    BOUNDARY_SUFFIX = b'\r\n'
    jpeg_quality = CONFIG.get('image_ops', {}).get('jpeg_quality', 80)
    encode_param = [cv2.IMWRITE_JPEG_QUALITY, jpeg_quality]

    for raw_frame in cam_instance.generate_raw_frames(camera_id):
        try:
            with _bbox_lock:
                bboxes = list(_bbox_state)

            display_frame = _draw_bboxes_on_frame(raw_frame, bboxes)

            success, encoded = cv2.imencode('.jpg', display_frame, encode_param)
            if not success:
                continue

            yield BOUNDARY_PREFIX + encoded.tobytes() + BOUNDARY_SUFFIX

        except Exception:
            pass





@app.route("/api/start_face_id", methods=["POST"])
def start_face_id():
    """Enable AI face-identification on the running pipeline."""
    status = ipc.start_face_identification()
    if status == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
        return jsonify({"status": "success", "message": "Face identification started"})
    desc = app_ipc.IPC_STATUS.DESCRIPTIONS.get(status, f"IPC error {status}")
    return jsonify({"status": "error", "message": desc}), 500


@app.route("/api/stop_face_id", methods=["POST"])
def stop_face_id():
    """Disable AI face-identification without stopping the pipeline."""
    status = ipc.stop_face_identification()
    if status == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
        return jsonify({"status": "success", "message": "Face identification stopped"})
    desc = app_ipc.IPC_STATUS.DESCRIPTIONS.get(status, f"IPC error {status}")
    return jsonify({"status": "error", "message": desc}), 500


@app.route("/api/start_pipeline", methods=["POST"])
def start_pipeline():
    """Instruct the C backend to initialise and start the SOM video pipeline."""
    status = ipc.start_pipeline()
    if status == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
        return jsonify({"status": "success", "message": "Pipeline started"})
    desc = app_ipc.IPC_STATUS.DESCRIPTIONS.get(status, f"IPC error {status}")
    return jsonify({"status": "error", "message": desc}), 500


@app.route("/api/stop_pipeline", methods=["POST"])
def stop_pipeline():
    """Instruct the C backend to stop the SOM video pipeline and release resources."""
    status = ipc.stop_pipeline()
    if status == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
        return jsonify({"status": "success", "message": "Pipeline stopped"})
    desc = app_ipc.IPC_STATUS.DESCRIPTIONS.get(status, f"IPC error {status}")
    return jsonify({"status": "error", "message": desc}), 500


@app.route("/api/generate_face_id", methods=["POST"])
def generate_face_id():
    status, op_status, face_id = ipc.generate_face_id_async()
    if status == app_ipc.IPC_STATUS.KT_APP_SUCCESS and op_status == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
        return jsonify({
            "status": "success",
            "face_id": face_id,
        })
    return jsonify({"status": "error", "message": "C Backend failed or timed out"}), 500




@app.route("/api/update_face_id", methods=["POST"])
def update_face_id_route():
    """
    Update or save a face ID record via IPC.
    Sends face_id and name to the C server to persist the profile.
    """
    data = request.get_json(silent=True) or {}
    face_id = data.get("face_id")
    name    = str(data.get("name", "")).strip() or "Unidentified Person"

    # face_id must be present; 0 is a valid enrolled ID (first person, uint32 from C backend)
    if face_id is None:
        return jsonify({"status": "error", "message": "face_id is required"}), 400
    
    if int(face_id) == 0:
        return jsonify({"status": "error", "message": "face_id cannot be 0"}), 400

    fid = int(face_id)

    try:
         # Refresh local cache from C backend, then refuse if this ID is already registered
        sync = ipc.list_face_profiles()
        if sync != app_ipc.IPC_STATUS.KT_APP_SUCCESS:
            return jsonify({
                "status": "error",
                "message": "Failed to refresh face profiles before save",
            }), 500


        success = ipc.update_face_profile(fid, name)
        if success == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
            return jsonify({
                "status": "success",
                "saved": {
                    "face_id": face_id,
                    "name": name,
                }
            })
        else:
            return jsonify({
                "status": "error",
                "message": "Failed to update face profile via IPC"
            }), 500
        
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('error_save_face_id', 'Error saving face ID: {error}').format(error=str(e))
        print(error_msg)
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 500


@app.route("/api/face_profiles", methods=["GET"])
def get_face_profiles():
    try:
        success = ipc.list_face_profiles()
        if success == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
            json_db_path = os.path.join(os.path.dirname(__file__), 'face_ids.json')
            if os.path.exists(json_db_path):
                with open(json_db_path, 'r') as f:
                    profiles = json.load(f)
                return jsonify({
                    "status": "success",
                    "profiles": profiles,
                    "count": len(profiles)
                })
            else:
                return jsonify({
                    "status": "success",
                    "profiles": [],
                    "count": 0
                })
        else:
            return jsonify({
                "status": "error",
                "message": "Failed to fetch face profiles from IPC server"
            }), 500
        
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('error_get_face_profiles', 'Error fetching face profiles: {error}').format(error=str(e))
        print(error_msg)
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 500


@app.route("/api/face_profiles/<int:face_id>", methods=["DELETE"])
def delete_face_profile(face_id):
    try:
        del_status, del_op = ipc.delete_face_id_async(int(face_id))
        if del_status == app_ipc.IPC_STATUS.KT_APP_SUCCESS and del_op == app_ipc.IPC_STATUS.KT_APP_SUCCESS:
            return jsonify({
                "status": "success",
                "message": f"Face profile {face_id} deleted successfully"
            })
        else:
            return jsonify({
                "status": "error",
                "message": f"Failed to delete face profile {face_id}"
            }), 500
        
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('error_delete_face_profile', 'Error deleting face profile: {error}').format(error=str(e))
        print(error_msg)
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 500





def cleanup_resources():
    """
    Releases all resources on shutdown: stops bbox threads and cameras.
    Idempotent — safe to call from signal handler, KeyboardInterrupt, finally,
    and atexit; only the first call does any work.
    """
    global shutdown_in_progress
    if shutdown_in_progress:
        return
    shutdown_in_progress = True

    print("\n[SHUTDOWN] Stopping bbox stale-detection thread...")
    _bbox_gen_stop.set()

    print("[SHUTDOWN] Releasing camera resources...")
    try:
        cam_instance.cleanup_resources()
    except Exception as e:
        print(f"[SHUTDOWN] Warning during camera cleanup: {e}")

    print("[SHUTDOWN] Cleanup complete.")


def signal_handler(signum, frame):
    """
    Handle SIGINT (Ctrl+C) and SIGTERM for graceful shutdown.
    Delegates entirely to cleanup_resources() which is idempotent.
    """
    signal_name = "SIGINT" if signum == signal.SIGINT else "SIGTERM"
    print(f"\n[SHUTDOWN] Received {signal_name} — shutting down gracefully...")
    cleanup_resources()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)
atexit.register(cleanup_resources)


@app.route("/")
def index():
    """Serve the main HTML page of the web application."""
    return render_template("index.html")


@app.route("/api/config")
def get_config():
    """API endpoint to serve configuration to frontend."""
    frontend_config = {
        'info': CONFIG.get('info', {}),
        'face_overlay': CONFIG.get('face_overlay', {}),
    }
    return jsonify(frontend_config)


@app.route("/api/live_detection")
def live_detection():
    """
    Latest primary face detection flags for the HMI.
    can_register_face: True when pipeline reports ideal=1 and face_id=0 (enrolment-ready).
    """
    with _bbox_lock:
        bboxes = list(_bbox_state)
    if not bboxes:
        return jsonify({
            "face_id": None,
            "ideal": None,
            "can_register_face": False,
        })
    primary = bboxes[0]
    fid = int(primary.get("face_id", 0))
    ideal = int(primary.get("ideal", 0))
    return jsonify({
        "face_id": fid,
        "ideal": ideal,
        "can_register_face": bool(ideal == 1 and fid == 0),
    })


@app.route("/video_feed")
def video_feed():
    """Legacy route redirects to CLNX camera feed for backward compatibility."""
    return video_feed_clnx()


@app.route("/video_feed/clnx")
def video_feed_clnx():
    """Serve MJPEG video stream from CLNX camera (USB Camera)."""
    http_codes = CONFIG.get('http_status_codes', {})
    mime_types = CONFIG.get('mime_types', {})
    no_content = http_codes.get('no_content', 204)
    ok = http_codes.get('ok', 200)
    
    if USB_CAMERA_ID not in cam_instance.available_cameras:
        return Response("", status=no_content, mimetype=mime_types.get('text_plain', 'text/plain'))
    running_status = CONFIG.get('status', {}).get('running', 'Running')
    if cam_instance.get_camera_status(USB_CAMERA_ID) == running_status:
        return Response(
            generate_frames_with_bbox(USB_CAMERA_ID),
            mimetype=mime_types.get('multipart_mixed', 'multipart/x-mixed-replace; boundary=frame'),
            status=ok,
        )
    else:
        return Response("", status=no_content, mimetype=mime_types.get('text_plain', 'text/plain'))


@app.route("/camera_control", methods=["POST"])
def camera_control():
    """Control camera start/stop for CLNX (USB) camera."""
    action = request.form.get("action")
    camera_type = request.form.get("camera_type", "clnx")

    if camera_type != "clnx":
        return jsonify(
            {"status": "error", "message": "camera_type must be clnx", "camera_type": camera_type, "action": action}
        ), 400

    camera_id = USB_CAMERA_ID

    if action not in ("start", "stop"):
        return jsonify(
            {"status": "error", "message": "action must be start or stop", "camera_type": camera_type, "action": action}
        ), 400

    try:
        if action == "start":
            cam_instance.start_camera(camera_id)
        else:
            cam_instance.stop_camera(camera_id)

        status_success = CONFIG.get('status', {}).get('success', 'success')
        return jsonify(
            {"status": status_success, "camera_type": camera_type, "action": action}
        )
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('error_camera_control', 'Error in camera_control: {error}').format(error=e)
        print(error_msg)
        traceback.print_exc()
        status_error = CONFIG.get('status', {}).get('error', 'error')
        return jsonify(
            {"status": status_error, "message": str(e), "camera_type": camera_type, "action": action}
        ), CONFIG.get('http_status_codes', {}).get('internal_server_error', 500)



@app.route("/camera_status/<camera_type>")
def camera_status(camera_type):
    """Return status for CLNX (USB) camera."""
    status_config = CONFIG.get('status', {})
    no_camera = status_config.get('no_camera_found', 'No Camera Found')

    if camera_type == "clnx":
        if USB_CAMERA_ID not in cam_instance.available_cameras:
            status = no_camera
        else:
            status = cam_instance.get_camera_status(USB_CAMERA_ID)
    else:
        status = CONFIG.get('status', {}).get('unknown', 'Unknown')

    return jsonify({"camera_type": camera_type, "status": status})



if __name__ == "__main__":
    try:
        server_config = CONFIG.get('server', {})
        host = server_config.get('host', '0.0.0.0')
        port = server_config.get('port', 5000)
        
        server_messages = CONFIG.get('server_messages', {})
        print(server_messages.get('starting_app', 'Starting Katana application...'))
        print(server_messages.get('server_available', 'Server available at: http://{host}:{port}').format(host=host, port=port))
        print(server_messages.get('press_ctrl_c', 'Press Ctrl+C to stop'))

        app.run(
            host=host,
            port=port,
            debug=server_config.get('debug', True),
            threaded=server_config.get('threaded', True),
            use_reloader=server_config.get('use_reloader', False)
        )

    except KeyboardInterrupt:
        # Flask/Werkzeug can raise KeyboardInterrupt directly on Ctrl+C
        # before our SIGINT handler fires — treat it identically.
        print("\n[SHUTDOWN] KeyboardInterrupt received — shutting down gracefully...")
        cleanup_resources()

    except Exception as e:
        server_messages = CONFIG.get('server_messages', {})
        error_msg = server_messages.get('error_occurred', 'Error: {error}').format(error=e)
        print(error_msg)
        cleanup_resources()

    finally:
        cleanup_resources()


