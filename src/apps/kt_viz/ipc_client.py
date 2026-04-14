"""
PYTHON CLIENT COMMUNICATING WITH C KT_APP SERVER OVER UNIX DOMAIN SOCKETS :

Bridges Python application logic to the KT-App C backend over Unix Domain Sockets.
Uses CFFI to map C structs directly from kt_app_cmds.h at runtime.
Two sockets are used: one for request/response commands and one for the live face-detection event stream.
"""

import socket
import os
import json
import threading
import time
import re
from cffi import FFI

# Single FFI instance shared across the whole module for CFFI
ffi = FFI()


# ERROR CODES FOR UNIX DOMAIN SOCKETS
class IPC_STATUS:
    KT_APP_SUCCESS = 0
    KT_APP_ERROR_GENERIC = -1
    KT_APP_ERROR_IO = -2
    KT_APP_ERROR_NOT_INITED = -3
    KT_APP_ERROR_UNSUPPORTED = -4
    KT_APP_ERROR_TIMEOUT = -5
    KT_APP_ERROR_QR_DECODE_FAIL = -100
    KT_APP_ERROR_FACE_ID_FAIL = -200
    KT_APP_ERROR_FACE_ALREADY_REGISTERED = -201
    KT_APP_ERROR_FACE_ID_GENERATION_IN_PROGRESS = -202
    KT_APP_ERROR_FACE_NOT_FOUND = -203
    KT_APP_RET_CODE_PENDING = 0xFFFFF  
    PY_ERR_COMMUNICATION_FAILURE = -999  


    # Log Level description for every status code 
    DESCRIPTIONS = {
        0: "SUCCESS: Operation completed.",
        -1: "ERROR: Generic system failure.",
        -2: "ERROR: SOM I/O failure.",
        -3: "ERROR: System not initialized.",
        -4: "ERROR: Feature unsupported.",
        -5: "ERROR: Hardware timeout.",
        -100: "ERROR: QR Decode failed.",
        -200: "ERROR: Face-ID generation failed.",
        -201: "ERROR: Face already registered.",
        -202: "ERROR: AI Task already in progress.",
        -203: "ERROR: Face not found in database.",
        0xFFFFF: "PENDING: AI Task active. Socket Active.please wait...",
        -999: "ERROR: Communication failure between Python and C",
    }

# How many times an async operation (generate / delete) is polled before timeout
ASYNC_OP_MAX_POLLS = 5

# Delay between consecutive polls of an async operation.
ASYNC_OP_POLL_DELAY_SEC = 1

# Dictionary to store Enum Commands and Values
KT_APP_CMD_ENUM = {}

#  Fallback mapping of invalid commands & ids in parsing failure case
KT_APP_CMD_FALLBACK = {
    "KT_APP_CMD_NONE": 0,
    "KT_APP_CMD_START_PIPELINE": 1,
    "KT_APP_CMD_START_FACE_IDENTIFICATION": 2,
    "KT_APP_CMD_GENERATE_FACE_ID_ASYNC": 3,
    "KT_APP_CMD_UPDATE_FACE_PROFILE": 4,
    "KT_APP_CMD_LIST_FACE_PROFILES": 5,
    "KT_APP_CMD_START_QR_MONITOR": 6,
    "KT_APP_CMD_STOP_PIPELINE": 7,
    "KT_APP_CMD_STOP_FACE_IDENTIFICATION": 8,
    "KT_APP_CMD_DELETE_FACE_ID_ASYNC": 9,
    "KT_APP_CMD_STOP_QR_MONITOR": 10,
    "KT_APP_CMD_EXIT": 11,
    "NR_KT_APP_CMDS": 12,
}



def parse_enum_kt_app_cmd(content):
    """
    Extracts the kt_app_cmd enum body from raw C header text and returns a
    dict mapping each enumerator name to its integer value.
    """
    out = {}
    m = re.search(r"enum\s+kt_app_cmd\s*\{([^}]+)\}", content, re.DOTALL)
    if not m:
        return out
    body = re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.DOTALL)
    for part in body.split(","):
        part = re.sub(r"//.*", "", part).strip()
        if not part:
            continue
        mm = re.match(r"^\s*(\w+)\s*=\s*(\d+)\s*U?\s*$", part)
        if mm:
            out[mm.group(1)] = int(mm.group(2))
    return out


def kt_app_cmd(name):
    """
    Map the integer value of a command by name from the parsed enum dict.
    """
    return KT_APP_CMD_ENUM[name]


# Function to parse the header file for cffi compatiblity
def parse_header():
    """
    Reads & parse kt_app_cmds.h performing following operations :
    - Build the command-ID enum map first; fall back to the fallback dict if parsing fails
    - remove single and multiline comments & pragma
    - replace #define macros with their values for cffi compatiblity
    - replace flexible array member faces[0] with faces[]
    - split the header at the pack pragma boundary so each group of structs gets the correct alignment when registered with CFFI.
    """
    global KT_APP_CMD_ENUM
    header_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "kt_app_cmds.h")
    try:
        with open(header_path, "r", encoding="utf-8") as f:
            content = f.read()

        KT_APP_CMD_ENUM = parse_enum_kt_app_cmd(content)
        if not KT_APP_CMD_ENUM:
            KT_APP_CMD_ENUM = dict(KT_APP_CMD_FALLBACK)

        defines = {}
        for m in re.finditer(r"^\s*#\s*define\s+(\w+)\s+(.+)$", content, re.MULTILINE):
            name, val = m.group(1), m.group(2).split("/*")[0].strip()
            val = re.sub(r"(\d+)U", r"\1", val)
            defines[name] = val

        def clean(raw):
            c = re.sub(r"^\s*#.*$", "", raw, flags=re.MULTILINE)
            c = re.sub(r"//.*", "", c)
            c = re.sub(r"/\*.*?\*/", "", c, flags=re.DOTALL)
            for _ in range(2):
                for name, val in defines.items():
                    if not val.startswith('"'):
                        c = re.sub(r"\b" + name + r"\b", val, c)
            c = c.replace("faces[0]", "faces[]")
            return c

        PACK_MARKER = "#pragma pack(push, 1)"
        pack_pos = content.find(PACK_MARKER)

        if pack_pos != -1:
            part_natural = clean(content[:pack_pos])
            part_packed = clean(content[pack_pos:])
            if part_natural.strip():
                ffi.cdef(part_natural, packed=False)
            if part_packed.strip():
                ffi.cdef(part_packed, packed=True)
        else:
            ffi.cdef(clean(content), packed=True)

        return {k: v.strip('"') for k, v in defines.items()}
    except Exception as e:
        print(f"[CFFI FATAL] Error parsing header: {e}")
        return {}


# Parsed socket paths and other macros from the header file
header_defs = parse_header()
REQ_RESP_SOCKET = header_defs.get("KT_APP_IPC_REQ_RESP_SOCKET", "/tmp/kt_app_req_resp.sock")
STREAM_SOCKET = header_defs.get("KT_APP_IPC_STREAM_SOCKET", "/tmp/kt_app_stream_socket")
JSON_DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "face_ids.json")
PATH_MAX = int(header_defs.get("PATH_MAX", 256))
KT_APP_MAX_FACE_NAME_LENGTH = PATH_MAX - 1
KT_APP_MAX_FACE_PROFILES_ON_DISK = int(header_defs.get("KT_APP_MAX_FACE_PROFILES_ON_DISK", 100))


#event callback function to continuously listen to the stream socket and print the face data
def event_callback(data, face_count, face_index):
    """
    Default debug callback invoked once per detected face per stream frame.
    """
    if face_count == 0:
        print("\n[EVENT] Frame is empty (No faces detected).")
        return

    face_id, left, top, right, bottom, hat, glasses, gloves, ideal = data
    if face_index == 1:
        print(f"\n[EVENT] Total Faces in Frame: {face_count}")
    print(f"  Face {face_index} : ID={face_id} BBox=[{left},{top},{right},{bottom}] || PPE=[{hat},{glasses},{gloves}] || Ideal=[{ideal}]")


# A centralised class to handle all the IPC operations for request response socket
class AppIPCManager:
    def __init__(self, clear_json_db=True):
        print(f"[IPC] Initializing with REQ_RESP_SOCKET: {REQ_RESP_SOCKET}")
        if not os.path.exists(REQ_RESP_SOCKET):
            try:
                os.makedirs(os.path.dirname(REQ_RESP_SOCKET), exist_ok=True)
            except Exception as e:
                print(f"[IPC] Warning: Could not create socket directory: {e}")
        if clear_json_db and os.path.exists(JSON_DB_PATH):
            os.remove(JSON_DB_PATH)

    def recv_exact(self, sock, n):
        """
        Reads exactly n bytes from a stream socket, accumulating partial reads
        until the full payload has arrived. Returns None if the connection
        is closed before all bytes are received.
        """
        buf = b""
        while len(buf) < n:
            chunk = sock.recv(n - len(buf))
            if not chunk:
                return None
            buf += chunk
        return buf

    def log_status(self, code, context):
        """
        Prints a formatted status line for any IPC operation and returns the
        status code unchanged so callers can use this as a pass-through.
        """
        desc = IPC_STATUS.DESCRIPTIONS.get(code, f"Unknown Code {code}")
        if code == IPC_STATUS.KT_APP_RET_CODE_PENDING:
            print(f"[ASYNC] {context} -> {desc}")
        elif code < 0:
            print(f"[FAILURE] {context} -> {desc}")
        else:
            print(f"[SUCCESS] {context} -> {desc}")
        return code


    def send_request(self, req_obj) -> tuple[int, ffi.CData | None]:
        """
        Sends a CFFI kt_app_request to the C backend and reads back the response
        into a CFFI kt_app_response object as per kt_app_cmds.h.
        
        helper api for all the request-response APIs in the class.
        """
        if not os.path.exists(REQ_RESP_SOCKET):
            return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None

        cmd_val = int(req_obj.cmd)
        
        # Determine the size of the request payload based on the command
        # For commands with request payload, the size is just sizeof(enum kt_app_cmd)
        request_payload_size = 0
        if cmd_val == kt_app_cmd("KT_APP_CMD_UPDATE_FACE_PROFILE"):
            request_payload_size = ffi.sizeof("struct _update_face_profile_request")
        elif cmd_val == kt_app_cmd("KT_APP_CMD_DELETE_FACE_ID_ASYNC"):
            request_payload_size = ffi.sizeof("struct _delete_face_id_async_request")
        

        # Total size to send: command + specific request payload
        send_buf_len = ffi.sizeof("enum kt_app_cmd") + request_payload_size
        send_buf = ffi.buffer(req_obj, send_buf_len)[:] # Get bytes from the CFFI struct directly

        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        # handling timeout time as per Async generate/delete 
        client.settimeout(max(10.0, ASYNC_OP_MAX_POLLS * ASYNC_OP_POLL_DELAY_SEC + 5.0))
        try:
            client.connect(REQ_RESP_SOCKET)
            client.sendall(send_buf)

            # Always read the 8-byte header: cmd(uint32) + ret_code(int32).
            hdr = self.recv_exact(client, ffi.sizeof("enum kt_app_cmd") + ffi.sizeof("enum kt_app_ret_code"))
            if not hdr:
                return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None

            # Create a kt_app_response struct to cast the header into
            resp = ffi.new("struct kt_app_response *")
            ffi.memmove(resp, hdr, len(hdr)) 
            
            resp_cmd = int(resp.cmd)
            ret_code = int(resp.ret_code)

            # verify the command sent and the command received are the same
            if resp_cmd != cmd_val:
                print(f"[VERIFY ERROR] Sent CMD {cmd_val}, C replied with CMD {resp_cmd}")
                return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None

            if cmd_val == kt_app_cmd("KT_APP_CMD_GENERATE_FACE_ID_ASYNC"):
                self.log_status(ret_code, "GENERATE_REQ")
            elif cmd_val == kt_app_cmd("KT_APP_CMD_DELETE_FACE_ID_ASYNC"):
                self.log_status(ret_code, "DELETE_REQ")

            # handling pending status for Async commands
            if ret_code == IPC_STATUS.KT_APP_RET_CODE_PENDING and cmd_val in (
                kt_app_cmd("KT_APP_CMD_GENERATE_FACE_ID_ASYNC"),
                kt_app_cmd("KT_APP_CMD_DELETE_FACE_ID_ASYNC"),
            ):
                hdr2 = self.recv_exact(client, ffi.sizeof("enum kt_app_cmd") + ffi.sizeof("enum kt_app_ret_code"))
                if not hdr2:
                    return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None
                ffi.memmove(resp, hdr2, len(hdr2))
                resp_cmd = int(resp.cmd)
                ret_code = int(resp.ret_code)
                if resp_cmd != cmd_val:
                    print(f"[VERIFY ERROR] Sent CMD {cmd_val}, C replied with CMD {resp_cmd}")
                    return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None
                self.log_status(
                    ret_code,
                    "AI_GENERATE" if cmd_val == kt_app_cmd("KT_APP_CMD_GENERATE_FACE_ID_ASYNC") else "DELETE FACE_ID_ASYNC",
                )

            # Handle commands with specific response payloads
            if cmd_val == kt_app_cmd("KT_APP_CMD_GENERATE_FACE_ID_ASYNC") and ret_code == IPC_STATUS.KT_APP_SUCCESS:
                # Expecting _generate_face_id_async_response (face_id + op_status)
                payload_size = ffi.sizeof("struct _generate_face_id_async_response")
                payload = self.recv_exact(client, payload_size)
                if not payload:
                    return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None
                ffi.memmove(ffi.addressof(resp.generate_face_id_async_response), payload, payload_size)
                
            elif cmd_val == kt_app_cmd("KT_APP_CMD_LIST_FACE_PROFILES") and ret_code == IPC_STATUS.KT_APP_SUCCESS:
                # Expecting face_count (uint32) + face_profiles array
                count_buf = self.recv_exact(client, ffi.sizeof("uint32_t"))
                if not count_buf:
                    return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None
                
                # Directly assign face_count to the response struct member
                resp.list_face_profiles_response.face_count = ffi.cast("uint32_t *", ffi.from_buffer(count_buf))[0]
                face_count = int(resp.list_face_profiles_response.face_count)
                
                # Read all face_profile structs 
                if face_count > 0:
                    profile_size = ffi.sizeof("struct kt_app_face_profile")
                    total_profiles_data_size = face_count * profile_size
                    
                    profiles_data = self.recv_exact(client, total_profiles_data_size)
                    if not profiles_data:
                        return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None
                    
                     # temporary buffer to receive all profiles
                    temp_profiles_array = ffi.new("struct kt_app_face_profile[]", face_count)
                    ffi.memmove(temp_profiles_array, profiles_data, total_profiles_data_size)
                    
                    # populate resp.list_face_profiles_response.face_profiles from temp_profiles_array
                    num_to_copy = min(face_count, KT_APP_MAX_FACE_PROFILES_ON_DISK)
                    ffi.memmove(resp.list_face_profiles_response.face_profiles, temp_profiles_array, num_to_copy * profile_size)
            
            # handling delete face id async command
            elif cmd_val == kt_app_cmd("KT_APP_CMD_DELETE_FACE_ID_ASYNC") and ret_code == IPC_STATUS.KT_APP_SUCCESS:
                payload_size = ffi.sizeof("struct _delete_face_id_async_response")
                payload = self.recv_exact(client, payload_size)
                if not payload:
                    return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None
                
                ffi.memmove(ffi.addressof(resp.delete_face_id_async_response), payload, payload_size)

            return ret_code, resp
        except Exception as e:
            print(f"[IPC EXCEPTION] send_request failed: {e}")
            return IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, None
        finally:
            client.close()

    # -----------------------------------------------------------------------
    # Request-Response APIs
    # -----------------------------------------------------------------------
    def start_pipeline(self):
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_START_PIPELINE")
        status, _ = self.send_request(req)
        return self.log_status(status, "START_PIPELINE")

    def stop_pipeline(self):
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_STOP_PIPELINE")
        status, _ = self.send_request(req)
        return self.log_status(status, "STOP_PIPELINE")

    def start_face_identification(self):
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_START_FACE_IDENTIFICATION")
        status, _ = self.send_request(req)
        return self.log_status(status, "START_FACE_IDENTIFICATION")

    def stop_face_identification(self):
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_STOP_FACE_IDENTIFICATION")
        status, _ = self.send_request(req)
        return self.log_status(status, "STOP_FACE_IDENTIFICATION")

    def start_qr_monitor(self):
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_START_QR_MONITOR")
        status, _ = self.send_request(req)
        return self.log_status(status, "START_QR_MONITOR")

    def stop_qr_monitor(self):
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_STOP_QR_MONITOR")
        status, _ = self.send_request(req)
        return self.log_status(status, "STOP_QR_MONITOR")

    # -----------------------------------------------------------------------
    # Async face-ID operations
    # -----------------------------------------------------------------------

    def generate_face_id_async(self):
        """
        Asks the C backend to capture and enrol a new face into the gallery.
        """
        if not os.path.exists(REQ_RESP_SOCKET):
            return self.log_status(IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, "GENERATE_REQ"), -1, -1

        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_GENERATE_FACE_ID_ASYNC")

        try:
            status, resp_ptr = self.send_request(req)
        except Exception as e:
            print(f"[IPC EXCEPTION] generate_face_id_async: {e}")
            return self.log_status(IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, "GENERATE_REQ"), -1, -1

        if status == IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE or resp_ptr is None:
            return self.log_status(IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, "GENERATE_REQ"), -1, -1

        ret_code = status

        face_id = -1
        op_status = -1
        if ret_code == IPC_STATUS.KT_APP_SUCCESS:
            face_id = int(resp_ptr.generate_face_id_async_response.face_id)
            op_status = int(resp_ptr.generate_face_id_async_response.op_status)
            if op_status == IPC_STATUS.KT_APP_SUCCESS:
                print(f"[GENERATE] New face_id={face_id}")
            return ret_code, op_status, face_id

        return ret_code, op_status, face_id


    def update_face_profile(self, face_id, name):
        """
        Updates the display name for an existing face ID in the C backend gallery.
        """
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_UPDATE_FACE_PROFILE")
        req.update_face_profile_request.face_id = face_id
        
        name_bytes = name.encode('utf-8')
        # Ensure name_bytes fits within KT_APP_MAX_FACE_NAME_LENGTH and is null-terminated
        ffi.memmove(req.update_face_profile_request.face_name, name_bytes, min(len(name_bytes), KT_APP_MAX_FACE_NAME_LENGTH - 1))
        req.update_face_profile_request.face_name[min(len(name_bytes), KT_APP_MAX_FACE_NAME_LENGTH - 1)] = b"\x00"

        status, _ = self.send_request(req)
        if status == IPC_STATUS.KT_APP_SUCCESS:
            self.list_face_profiles()   # keep JSON Database updated
        return self.log_status(status, "UPDATE_PROFILE")


    def list_face_profiles(self):
        """
        Retrieves the complete face-profile list from the C backend and writes
        it to the local face_ids.json file.
        """
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_LIST_FACE_PROFILES")
        status, resp_ptr = self.send_request(req)
        
        if status == IPC_STATUS.KT_APP_SUCCESS and resp_ptr:
            face_count = int(resp_ptr.list_face_profiles_response.face_count)
            profiles = []
            
            # Iterate through the fixed-size array within the response struct
            for i in range(min(face_count, KT_APP_MAX_FACE_PROFILES_ON_DISK)):
                p = resp_ptr.list_face_profiles_response.face_profiles[i]
                # Decode the C char array to a Python string
                name_str = ffi.string(p.face_name).decode('utf-8', errors='ignore')
                profiles.append({"face_id": int(p.face_id), "name": name_str})
            
            with open(JSON_DB_PATH, 'w') as f:
                json.dump(profiles, f, indent=2)
            print(f"[SYNC] {face_count} profiles saved.")
            return IPC_STATUS.KT_APP_SUCCESS
        return self.log_status(status, "LIST_PROFILES")


    def delete_face_id_async(self, face_id):
        """
        Asks the C backend to remove a face ID from the DATABASE asynchronously.
        """
        if not os.path.exists(REQ_RESP_SOCKET):
            return self.log_status(IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, "DELETE_REQ"), -1

        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_DELETE_FACE_ID_ASYNC")
        req.delete_face_id_async_request.face_id = face_id

        try:
            status, resp_ptr = self.send_request(req)
        except Exception as e:
            print(f"[IPC EXCEPTION] delete_face_id_async: {e}")
            return self.log_status(IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, "DELETE_REQ"), -1

        if status == IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE or resp_ptr is None:
            return self.log_status(IPC_STATUS.PY_ERR_COMMUNICATION_FAILURE, "DELETE_REQ"), -1

        ret_code = status

        op_status = -1
        if ret_code == IPC_STATUS.KT_APP_SUCCESS:
            op_status = int(resp_ptr.delete_face_id_async_response.op_status)
            if op_status == IPC_STATUS.KT_APP_SUCCESS:
                self.list_face_profiles()   # keep JSON Database updated

        return ret_code, op_status



    def exit_app(self):
        """Sends the EXIT command to the C backend, signalling it to shut down cleanly."""
        req = ffi.new("struct kt_app_request *")
        req.cmd = kt_app_cmd("KT_APP_CMD_EXIT")
        status, _ = self.send_request(req)
        return self.log_status(status, "EXIT")



    def start_event_listener(self, callback):
        """
        Start the event listener thread to continuously listen to the stream socket and obtain the data
        """
        def listener_worker():
            sz = ffi.sizeof("struct kt_app_face_data")
            while True:
                if not os.path.exists(STREAM_SOCKET):
                    if not os.path.exists(REQ_RESP_SOCKET): #
                        stream_sock_dir = os.path.dirname(STREAM_SOCKET)
                        if stream_sock_dir and not os.path.exists(stream_sock_dir):
                            try:
                                os.makedirs(stream_sock_dir, exist_ok=True)
                                print(f"[STREAM] Created directory for stream socket: {stream_sock_dir}")
                            except Exception as e:
                                print(f"[STREAM] Error creating directory {stream_sock_dir}: {e}")
                                time.sleep(1)
                                continue
                        else:
                            print(f"[STREAM] Stream socket {STREAM_SOCKET} not found, retrying...")
                    time.sleep(1)
                    continue
                sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                try:
                    sock.connect(STREAM_SOCKET)
                    print(f"[STREAM] Connected to stream socket: {STREAM_SOCKET}")
                    while True:
                        h = self.recv_exact(sock, ffi.sizeof("uint32_t"))
                        if not h:
                            print("[STREAM] Server closed connection — header not received (0 bytes)")
                            break
                        cnt = ffi.cast("uint32_t *", ffi.from_buffer(h))[0]
                        if cnt == 0:
                            print("[STREAM] RX empty frame (face_count=0) — overlay cleared")
                            callback((0, -1, -1, -1, -1, 0, 0, 0, 0), 0, 0)
                            continue
                        print(f"[STREAM] RX frame: {int(cnt)} face(s)")
                        
                        total_face_data_size = int(cnt) * sz
                        p = self.recv_exact(sock, total_face_data_size)
                        if not p:
                            print(f"[STREAM] Server closed connection mid-frame — payload missing for {int(cnt)} face(s)")
                            break
                        
                        # Cast the raw bytes directly to an array of face structs
                        faces_array = ffi.cast(f"struct kt_app_face_data[{int(cnt)}]", ffi.from_buffer(p))
                        
                        for i in range(int(cnt)):
                            f = faces_array[i]
                            print(
                                f"[STREAM]   face[{i+1}/{int(cnt)}] "
                                f"id={int(f.face_id)}  "
                                f"bbox=[{int(f.left)},{int(f.top)},{int(f.right)},{int(f.bottom)}]  "
                                f"ppe=[hat={int(f.safety_hat_on)} glasses={int(f.safety_glasses_on)} gloves={int(f.safety_gloves_on)}]  "
                                f"ideal={int(f.ideal_person)}"
                            )
                            callback((f.face_id, f.left, f.top, f.right, f.bottom,
                                      f.safety_hat_on, f.safety_glasses_on,
                                      f.safety_gloves_on, f.ideal_person), cnt, i+1)
                except Exception as e:
                    print(f"[STREAM] Connection error: {e}")
                finally:
                    sock.close()
                    print(f"[STREAM] Disconnected from stream socket, reconnecting in 2s...")
                    time.sleep(2)

        threading.Thread(target=listener_worker, daemon=True).start()
