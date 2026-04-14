/**
 * ============================================================================
 * EDGE HUB - Main Application Script
 * ============================================================================
 * 
 * This script provides the core functionality for the Edge HUB web application,
 * including:
 * 
 * 1. Configuration Management
 *    - Loads configuration from server API endpoint
 *    - Merges with default configuration values
 *    - Provides fallback defaults if server config unavailable
 * 
 * 2. Sensor Dashboard
 *    - Real-time sensor data visualization using Plotly.js
 *    - Four multi-line plots: Temperature, Voltage, Current, Power
 *    - Metric cards with trend indicators (up/down/neutral)
 *    - Automatic data updates at configurable intervals
 * 
 * 3. Live Camera Streaming
 *    - Support for CPNX (Pi Camera) and CLNX (USB Camera)
 *    - Dual camera mode with synchronized streaming
 *    - Camera control (start/stop) via API
 *    - Real-time status checking and error handling
 * 
 * 4. Image Operations
 *    - Capture images from GARD system (supports RAW/bin and standard image formats)
 *    - Save images to client (Downloads/edgeHUB) or server
 *    - File System Access API integration for client-side saves
 *    - RAW/bin format support with binary-to-PNG conversion for display
 *    - Image format detection and conversion (PNG, JPEG, BMP, RAW/bin)
 *    - Anomaly status tagging (normal/anomaly) for image classification
 *    - Configurable filename generation with timestamps and classification tags
 * 
 * 5. UI Navigation & Layout
 *    - Tab-based navigation (Sensor Dashboard, Live Streaming Dashboard, Image Operations Dashboard)
 *    - Modal dialogs for thresholds, ISP settings, circuit viewer
 *    - Responsive layout management
 * 
 * 6. Data Management
 *    - LocalStorage for saving user preferences (save paths)
 *    - Sliding window data management for plots (configurable window size)
 *    - Trend tracking for metric values
 * 
 * Dependencies:
 *    - Plotly.js (for graph visualization)
 *    - Font Awesome (for icons)
 *    - Modern browser with ES6+ support
 * 
 * ============================================================================
 */

document.addEventListener('DOMContentLoaded', async () => {
    // ========================================================================
    // SECTION 1: CONFIGURATION LOADING
    // ========================================================================
    
    let CONFIG = {};
    try {
        const configEndpoint = '/api/config';
        const configResponse = await fetch(configEndpoint);
        if (configResponse.ok) {
            CONFIG = await configResponse.json();
        } else {
            console.warn('Failed to load config, using defaults');
        }
    } catch (error) {
        console.warn('Error loading config:', error, 'using defaults');
    }
    
    // Default configuration values
    const DEFAULT_CONFIG = {
        cameras: { 
            usb: { width: 1920, height: 1080, display_name: 'Live Video Feed through CLNX USB Camera' } 
        },
        paths: { 
            default_server_save_path: '/home/lattice/Downloads/edgeHUB/', 
            default_client_save_path: 'Downloads', 
            save_folder_name: 'edgeHUB' 
        },
        image_ops: { 
            filename_prefix: 'edgehub_imageops', 
            timestamp_format: 'YYYY-MM-DDTHH-mm-ss', 
            image_format: 'png' 
        },
        api_endpoints: { 
            capture_image: '/api/capture_image_from_gard', 
            save_image: '/api/save_image', 
            video_feed_clnx: '/video_feed/clnx' 
        },
        local_storage_keys: { 
            client_save_path: 'edgeHUB_client_save_path', 
            server_save_path: 'edgeHUB_server_save_path' 
        },
        messages: { 
            info_text: 'Image will be downloaded in <strong>edgeHUB</strong> folder at the given location. By default it is set to Downloads folder.<br>Filename will be auto-generated as: <code style="color: var(--lattice-yellow); font-size: 0.85rem;">edgehub_imageops_&lt;classification&gt;_&lt;timestamp&gt;.png</code>' 
        },
        data_window_size: 40,
        face_overlay: {
            ideal_bgr: [0, 255, 0],
            non_ideal_palette_bgr: [],
            box_thickness_px: { not_ideal: 3, ideal: 20 },
            ppe_badge_bgr: [],
        },
    };

    function bgrTripletToRgb(bgr) {
        if (!Array.isArray(bgr) || bgr.length !== 3) return null;
        const b = Math.max(0, Math.min(255, Number(bgr[0]) || 0));
        const g = Math.max(0, Math.min(255, Number(bgr[1]) || 0));
        const r = Math.max(0, Math.min(255, Number(bgr[2]) || 0));
        return { r, g, b };
    }

    /** Maps config face_overlay.ideal_bgr (OpenCV BGR) to --face-ideal / --face-ideal-dark for UI. */
    function applyFaceOverlayCss(fo) {
        if (!fo || !Array.isArray(fo.ideal_bgr) || fo.ideal_bgr.length !== 3) return;
        const rgb = bgrTripletToRgb(fo.ideal_bgr);
        if (!rgb) return;
        const root = document.documentElement;
        root.style.setProperty('--face-ideal', `rgb(${rgb.r},${rgb.g},${rgb.b})`);
        const dr = Math.round(rgb.r * 0.72);
        const dg = Math.round(rgb.g * 0.72);
        const db = Math.round(rgb.b * 0.72);
        root.style.setProperty('--face-ideal-dark', `rgb(${dr},${dg},${db})`);
    }
    
    // Merge server config with defaults
    CONFIG = { ...DEFAULT_CONFIG, ...CONFIG };
    if (!CONFIG.face_overlay) CONFIG.face_overlay = { ...DEFAULT_CONFIG.face_overlay };
    else CONFIG.face_overlay = { ...DEFAULT_CONFIG.face_overlay, ...CONFIG.face_overlay };
    applyFaceOverlayCss(CONFIG.face_overlay);
    
    // ========================================================================
    // SECTION 2: DOM ELEMENT SELECTION
    // ========================================================================
    
    const tabPanels = document.querySelectorAll('.tab-panel');
    const dashboardContainer = document.querySelector('.dashboard-container');
    const infoBtn = document.getElementById('info-btn');
    
    
    // ========================================================================
    // SECTION 3: STATE & DATA MANAGEMENT
    // ========================================================================
    
    // Polling interval control for live detection
    let dataUpdateInterval = null;
    
    
    // ========================================================================
    // SECTION 4: LIVE DETECTION POLLING
    // ========================================================================
    
    /**
     * Polls /api/live_detection and enables Register when enrolment is allowed (ideal=1, face_id=0).
     */
    const updateDashboard = async () => {
        try {
            const detRes = await fetch('/api/live_detection');
            if (detRes.ok) {
                const det = await detRes.json();
                const canRegister = Boolean(det.can_register_face);
                document.querySelectorAll('.gen-face-id-btn').forEach(btn => {
                    if (!btn.dataset.busy) btn.disabled = !canRegister;
                });

                const activeIds = [];
                if (det.face_id != null && det.face_id !== 0) activeIds.push(det.face_id);
                if (Array.isArray(det.detected_face_ids)) {
                    det.detected_face_ids.forEach(id => { if (id !== 0 && !activeIds.includes(id)) activeIds.push(id); });
                }
                if (typeof window._refreshProfileCards === 'function') {
                    window._refreshProfileCards(activeIds);
                }
            }
        } catch (_) { /* offline */ }
    };

    function startDataUpdates() {
        if (dataUpdateInterval) return;
        updateDashboard();
        const updateInterval = CONFIG.delays?.data_update_interval || 1000;
        dataUpdateInterval = setInterval(updateDashboard, updateInterval);
    }

    function stopDataUpdates() {
        if (dataUpdateInterval) {
            clearInterval(dataUpdateInterval);
            dataUpdateInterval = null;
        }
    }
    
    // ========================================================================
    // SECTION 5: CAMERA CONTROL FUNCTIONS
    // ========================================================================
    
    /**
     * Shows the video stream and hides the stopped message.
     * 
     * This function updates the UI to display an active camera stream by making
     * the video element visible and hiding the "stopped" message element.
     * It safely handles null elements by checking for their existence before
     * manipulating their display properties.
     * 
     * @param {HTMLElement} videoElement - Video element to show
     * @param {HTMLElement} msgElement - Message element to hide
     */
    function showStream(videoElement, msgElement) {
        if (videoElement) videoElement.style.display = 'block';
        if (msgElement) msgElement.style.display = 'none';
    }
    
    /**
     * Hides the video stream and shows the stopped message.
     * 
     * This function updates the UI to indicate a stopped camera by hiding the
     * video element and displaying the "stopped" message. It safely handles null
     * elements by checking for their existence before manipulating their display
     * properties.
     * 
     * @param {HTMLElement} videoElement - Video element to hide
     * @param {HTMLElement} msgElement - Message element to show
     */
    function showStoppedMsg(videoElement, msgElement) {
        if (videoElement) {
            videoElement.src = '';          // kill the MJPEG connection
            videoElement.style.display = 'none';
        }
        if (msgElement) msgElement.style.display = 'block';
    }
    
    /**
     * Displays a camera error message.
     * 
     * This function handles error display for camera operations by hiding the
     * video element and showing the error message in the stopped message element.
     * It safely handles null elements and updates the message text content with
     * the provided error message.
     * 
     * @param {string} cameraType - 'cpnx' or 'clnx'
     * @param {string} message - Error message to display
     */
    function showCameraError(cameraType, message) {
        const videoElement = document.getElementById(`${cameraType}-video`);
        const msgElement = document.querySelector(`#${cameraType}-camera-panel .camera-stopped-msg`);
        
        if (videoElement) videoElement.style.display = 'none';
        if (msgElement) {
            msgElement.textContent = message;
            msgElement.style.display = 'block';
        }
    }

    const hiddenClass = 'is-hidden';
    const streamRunningState = { clnx: false };

    function setStreamButtonsState(cameraType, isRunning) {
        const startBtn  = document.querySelector(`.start-stream-btn[data-camera="${cameraType}"]`);
        const stopBtn   = document.querySelector(`.stop-stream-btn[data-camera="${cameraType}"]`);

        if (startBtn)  startBtn.classList.toggle(hiddenClass, isRunning);
        if (stopBtn)   stopBtn.classList.toggle(hiddenClass, !isRunning);

        streamRunningState[cameraType] = isRunning;
    }
    
    /**
     * Checks the status of all cameras and updates the UI accordingly.
     * 
     * This function queries the camera status API for both CPNX and CLNX cameras
     * and updates the UI based on their current status. It handles three states:
     * "No Camera Found" (hides video, shows error message), "Running" (shows video
     * with fresh timestamp to prevent caching, hides message), and "Stopped" (hides
     * video, shows stopped message). The function makes parallel requests for both
     * cameras and handles errors gracefully.
     */
    function checkCameraStatus() {
        const cameraType = 'clnx';
        fetch(`/camera_status/${cameraType}`)
            .then(response => response.json())
            .then(data => {
                const videoElement = document.getElementById('clnx-video');
                const msgElement = document.querySelector('#clnx-camera-panel .camera-stopped-msg');
                const isRunning = data.status === 'Running';

                if (data.status === 'No Camera Found') {
                    if (videoElement) videoElement.style.display = 'none';
                    if (msgElement) {
                        msgElement.textContent = 'No Camera Found';
                        msgElement.style.display = 'block';
                    }
                } else if (data.status === 'Running') {
                    if (videoElement) {
                        videoElement.src = `/video_feed/clnx?` + new Date().getTime();
                        videoElement.style.display = 'block';
                    }
                    if (msgElement) msgElement.style.display = 'none';
                } else {
                    if (videoElement) videoElement.style.display = 'none';
                    if (msgElement) {
                        msgElement.textContent = "Camera is stopped. Click 'Start Stream' to view.";
                        msgElement.style.display = 'block';
                    }
                }

                setStreamButtonsState(cameraType, isRunning);
            })
            .catch(error => {
                console.error('Error checking camera status:', error);
            });
    }
    
    /**
     * Sends a control command to the camera API (start/stop).
     * 
     * This function sends a POST request to the camera control API endpoint with
     * the specified action (start or stop) and camera type. It handles the response
     * by either updating the camera UI on success or displaying an error message
     * on failure. The function uses form-encoded data and handles network errors
     * gracefully by showing appropriate error messages to the user.
     * 
     * @param {string} action - Action to perform ('start' or 'stop')
     * @param {string} cameraType - 'cpnx' or 'clnx'
     */
    function controlCamera(action, cameraType) {
        fetch('/camera_control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `action=${action}&camera_type=${cameraType}`
        })
        .then(response => response.json())
        .then(data => {
            console.log(`Camera ${cameraType} ${action}:`, data);
            if (data.status === 'error') {
                showCameraError(cameraType, data.message || 'Failed to control camera');
            } else {
                // For stop: update UI immediately without waiting for status
                if (action === 'stop') {
                    updateCameraUI(cameraType, 'stop');
                    return;
                }
                setTimeout(() => updateCameraUI(cameraType, action), 300);
            }
        })
        .catch(error => {
            console.error(`Error controlling ${cameraType} camera:`, error);
            showCameraError(cameraType, error.message || 'Failed to control camera');
        });
    }
    
    /**
     * Updates the camera UI from server status (cpnx or clnx only).
     */
    function updateCameraUI(cameraType, action) {
        if (action === 'stop') {
            const videoElement = document.getElementById(`${cameraType}-video`);
            const msgElement = document.querySelector(`#${cameraType}-camera-panel .camera-stopped-msg`);
            showStoppedMsg(videoElement, msgElement);
            setStreamButtonsState(cameraType, false);
            return;
        }

        fetch(`/camera_status/${cameraType}`)
            .then(response => response.json())
            .then(data => {
                const videoElement = document.getElementById(`${cameraType}-video`);
                const msgElement = document.querySelector(`#${cameraType}-camera-panel .camera-stopped-msg`);
                const isRunning = data.status === 'Running';

                if (data.status === 'No Camera Found') {
                    showCameraError(cameraType, 'No Camera Found');
                } else if (data.status === 'Running') {
                    if (videoElement) {
                        videoElement.src = `${CONFIG.api_endpoints.video_feed_clnx}?` + new Date().getTime();
                        showStream(videoElement, msgElement);
                    }
                } else {
                    showStoppedMsg(videoElement, msgElement);
                }

                setStreamButtonsState(cameraType, isRunning);
            })
            .catch(error => {
                console.error(`Error updating camera UI for ${cameraType}:`, error);
            });
    }

    // Auto-start camera on page load
    controlCamera('start', 'clnx');

    // Attach event listeners to camera control buttons
    document.querySelectorAll('.start-stream-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            controlCamera('start', btn.getAttribute('data-camera'));
        });
    });

    document.querySelectorAll('.stop-stream-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            controlCamera('stop', btn.getAttribute('data-camera'));
        });
    });

    // Start / Stop Face ID buttons
    document.querySelectorAll('.start-face-id-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const camera = btn.getAttribute('data-camera');
            fetch('/api/start_face_id', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ camera })
            })
            .then(r => r.json())
            .then(res => {
                if (res.status === 'success') showNotif('Face ID started.', 'success');
                else showNotif(res.message || 'Failed to start Face ID.', 'error');
            })
            .catch(() => showNotif('Error starting Face ID.', 'error'));
        });
    });

    document.querySelectorAll('.stop-face-id-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const camera = btn.getAttribute('data-camera');
            fetch('/api/stop_face_id', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ camera })
            })
            .then(r => r.json())
            .then(res => {
                if (res.status === 'success') showNotif('Face ID stopped.', 'success');
                else showNotif(res.message || 'Failed to stop Face ID.', 'error');
            })
            .catch(() => showNotif('Error stopping Face ID.', 'error'));
        });
    });

    document.querySelectorAll('.start-pipeline-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const camera = btn.getAttribute('data-camera');
            fetch('/api/start_pipeline', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ camera })
            })
            .then(r => r.json())
            .then(res => {
                if (res.status === 'success') showNotif('Pipeline started.', 'success');
                else showNotif(res.message || 'Failed to start pipeline.', 'error');
            })
            .catch(() => showNotif('Error starting pipeline.', 'error'));
        });
    });

    document.querySelectorAll('.stop-pipeline-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const camera = btn.getAttribute('data-camera');
            fetch('/api/stop_pipeline', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ camera })
            })
            .then(r => r.json())
            .then(res => {
                if (res.status === 'success') showNotif('Pipeline stopped.', 'success');
                else showNotif(res.message || 'Failed to stop pipeline.', 'error');
            })
            .catch(() => showNotif('Error stopping pipeline.', 'error'));
        });
    });

    // ── Shared helpers: custom confirm + center notification ─────────────────
    /**
     * showConfirm(message) → Promise<boolean>
     * Shows a beautiful dark confirm dialog. Resolves true = OK, false = Cancel.
     */
    function showConfirm(message) {
        return new Promise(resolve => {
            const overlay   = document.getElementById('fid-confirm-overlay');
            const msgEl     = document.getElementById('fid-confirm-msg');
            const okBtn     = document.getElementById('fid-confirm-ok');
            const cancelBtn = document.getElementById('fid-confirm-cancel');
            msgEl.textContent = message;
            overlay.style.display = 'flex';
            function finish(result) {
                overlay.style.display = 'none';
                okBtn.removeEventListener('click', onOk);
                cancelBtn.removeEventListener('click', onCancel);
                overlay.removeEventListener('click', onBackdrop);
                resolve(result);
            }
            function onOk()      { finish(true);  }
            function onCancel()  { finish(false); }
            function onBackdrop(e) { if (e.target === overlay) finish(false); }
            okBtn.addEventListener('click', onOk);
            cancelBtn.addEventListener('click', onCancel);
            overlay.addEventListener('click', onBackdrop);
        });
    }

    /**
     * showNotif(message, type = 'success' | 'error')
     * Shows a centered auto-dismiss notification dialog.
     */
    function showNotif(message, type = 'success') {
        const overlay = document.getElementById('fid-notif-overlay');
        const msgEl   = document.getElementById('fid-notif-msg');
        const iconEl  = document.getElementById('fid-notif-icon');
        msgEl.textContent = message;
        iconEl.innerHTML  = type === 'error'
            ? '<i class="fas fa-times-circle"></i>'
            : '<i class="fas fa-check-circle"></i>';
        iconEl.className  = 'fid-notif-icon ' + (type === 'error' ? 'fid-notif-icon--error' : 'fid-notif-icon--success');
        overlay.style.display = 'flex';
        clearTimeout(overlay._autoClose);
        overlay._autoClose = setTimeout(() => { overlay.style.display = 'none'; }, 2200);
        overlay.onclick = e => { if (e.target === overlay) overlay.style.display = 'none'; };
    }

    // ── Face ID button wiring ─────────────────────────────────────────────────
    (function initFaceId() {
        // Cached result from the last generate call
        let _lastFaceIdResult = null;
        let _lastCamera = 'clnx';   // track which camera button was clicked

        const modal      = document.getElementById('face-id-modal');
        const fidIdVal   = document.getElementById('fid-id-val');
        const fidNameIn  = document.getElementById('fid-name-input');
        const fidNameRow = document.getElementById('fid-name-row');
        const okBtn      = document.getElementById('fid-ok-btn');
        const cancelBtn  = document.getElementById('fid-cancel-btn');

        function openModal(result) {
            _lastFaceIdResult = result;
            // Display the numeric face ID
            fidIdVal.textContent = result.face_id;

            fidNameIn.value = '';
            // face_id is a uint32 from C backend; 0 is a valid enrolled ID (first person).
            // Only treat it as invalid when the server explicitly signals failure (null / undefined).
            const isValidId = result.face_id && Number(result.face_id) !== 0;
            fidNameRow.style.display = isValidId ? '' : 'none';
            modal.style.display = 'flex';
            if (isValidId) fidNameIn.focus();
        }

        function closeModal() {
            modal.style.display = 'none';
            _lastFaceIdResult = null;
        }

        // Click outside dialog → cancel
        modal.addEventListener('click', e => { if (e.target === modal) closeModal(); });

        cancelBtn.addEventListener('click', closeModal);

        okBtn.addEventListener('click', () => {
            if (!_lastFaceIdResult) return;
            // face_id is a uint32 from C backend; 0 is a valid enrolled ID (first person).
            // Only block save when face_id is null/undefined (server signalled failure).
            const faceId = Number(_lastFaceIdResult.face_id);
            if (!faceId || faceId === 0) {
                closeModal();
                return;
            }
            // Send only face_id and name to the server
            const payload = {
                face_id: faceId,
                name:    fidNameIn.value.trim(),
            };
            fetch('/api/update_face_id', {
                method:  'POST',
                headers: { 'Content-Type': 'application/json' },
                body:    JSON.stringify(payload),
            })
                .then(r => r.json())
                .then(res => {
                    if (res.status === 'success') {
                        closeModal();
                        showNotif(`✔ Face ID ${payload.face_id} saved as "${payload.name || 'Unidentified Person'}"`);
                    } else {
                        showNotif('Save failed: ' + (res.message || 'unknown error'), 'error');
                    }
                })
                .catch(err => alert('Network error: ' + err));
        });

        // Allow Enter key to submit
        fidNameIn.addEventListener('keydown', e => {
            if (e.key === 'Enter') okBtn.click();
            if (e.key === 'Escape') closeModal();
        });

        // Wire every Generate Face ID button
        // Starts disabled; polling enables it when ideal=1 AND face_id=0
        document.querySelectorAll('.gen-face-id-btn').forEach(btn => {
            btn.disabled = true;
            btn.addEventListener('click', () => {
                _lastCamera = btn.dataset.camera || 'clnx';
                btn.dataset.busy = '1';
                btn.disabled = true;
                const orig = btn.innerHTML;
                btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Generating…';
                fetch('/api/generate_face_id', { method: 'POST' })
                    .then(r => r.json())
                    .then(res => {
                        delete btn.dataset.busy;
                        btn.disabled = false;
                        btn.innerHTML = orig;
                        if (res.status === 'success') {
                            openModal(res);
                        } else {
                            alert('Face ID generation failed: ' + (res.message || 'unknown error'));
                        }
                    })
                    .catch(err => {
                        delete btn.dataset.busy;
                        btn.disabled = false;
                        btn.innerHTML = orig;
                        alert('Network error: ' + err);
                    });
            });
        });
    })();

    // ── Delete Face ID button wiring ──────────────────────────────────────────
    (function initDeleteFaceId() {
        const modal      = document.getElementById('del-face-modal');
        const tbody      = document.getElementById('del-fid-tbody');
        const tableWrap  = document.getElementById('del-fid-table-wrap');
        const loading    = document.getElementById('del-fid-loading');
        const empty      = document.getElementById('del-fid-empty');
        const countEl    = document.getElementById('del-fid-count');
        const closeBtn   = document.getElementById('del-fid-close-btn');
        const refreshBtn = document.getElementById('del-fid-refresh-btn');

        function openModal() {
            modal.style.display = 'flex';
            loadProfiles();
        }

        function closeModal() {
            modal.style.display = 'none';
        }

        modal.addEventListener('click', e => { if (e.target === modal) closeModal(); });
        closeBtn.addEventListener('click', closeModal);
        refreshBtn.addEventListener('click', loadProfiles);

        function showToast(msg) {
            showNotif(msg);
        }

        function deleteRow(id, btn, tr) {
            showConfirm(`Delete face profile ID ${id}? This cannot be undone.`).then(confirmed => {
                if (!confirmed) return;
                btn.disabled = true;
                btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i>';
                fetch(`/api/face_profiles/${id}`, { method: 'DELETE' })
                    .then(r => r.json())
                    .then(res => {
                        if (res.status === 'success') {
                            tr.classList.add('del-fid-row-removing');
                            setTimeout(() => {
                                tr.remove();
                                const remaining = tbody.querySelectorAll('tr').length;
                                if (!remaining) { tableWrap.style.display = 'none'; empty.style.display = ''; }
                                else { countEl.textContent = `${remaining} profile${remaining !== 1 ? 's' : ''} registered`; }
                            }, 320);
                            showNotif(`Face ID ${id} deleted successfully`);
                        } else {
                            showNotif('Delete failed: ' + (res.message || 'unknown'), 'error');
                            btn.disabled = false;
                            btn.innerHTML = '<i class="fas fa-trash-alt"></i> Delete';
                        }
                    })
                    .catch(err => {
                        showNotif('Network error: ' + err, 'error');
                        btn.disabled = false;
                        btn.innerHTML = '<i class="fas fa-trash-alt"></i> Delete';
                    });
            });
        }

        function loadProfiles() {
            tableWrap.style.display = 'none';
            empty.style.display     = 'none';
            loading.style.display   = '';
            tbody.innerHTML = '';

            fetch('/api/face_profiles')
                .then(r => r.json())
                .then(res => {
                    loading.style.display = 'none';
                    const profiles = res.profiles || [];
                    if (!profiles.length) { empty.style.display = ''; return; }

                    profiles.forEach((p, idx) => {
                        const tr = document.createElement('tr');
                        tr.className = 'del-fid-tr';
                        tr.innerHTML = `
                            <td class="col-id">
                                <span class="del-fid-badge">${p.face_id}</span>
                            </td>
                            <td class="col-name">
                                <i class="fas fa-user del-fid-user-icon"></i>
                                <span>${p.name || '<em>Unnamed</em>'}</span>
                            </td>
                            <td class="col-action">
                                <button class="del-fid-del-btn" data-id="${p.face_id}">
                                    <i class="fas fa-trash-alt"></i> Delete
                                </button>
                            </td>`;
                        const btn = tr.querySelector('.del-fid-del-btn');
                        btn.addEventListener('click', () => deleteRow(p.face_id, btn, tr));
                        tbody.appendChild(tr);
                    });

                    countEl.textContent = `${profiles.length} profile${profiles.length !== 1 ? 's' : ''} registered`;
                    tableWrap.style.display = '';
                })
                .catch(() => { loading.style.display = 'none'; empty.style.display = ''; });
        }

        document.querySelectorAll('.del-face-id-btn').forEach(btn => {
            btn.addEventListener('click', openModal);
        });
    })();

    // ── Registered Profiles Dropdown ──────────────────────────────────────
    let _activeFaceIds = new Set();

    (function initProfilesDropdown() {
        const toggle     = document.getElementById('clnx-profiles-toggle');
        const dropdown   = document.getElementById('clnx-profiles-dropdown');
        const loader     = document.getElementById('clnx-profiles-loader');
        const empty      = document.getElementById('clnx-profiles-empty');
        const list       = document.getElementById('clnx-profiles-list');
        const countBadge = document.getElementById('clnx-profiles-count');
        if (!toggle || !dropdown) return;

        let loaded = false;
        let _fetchInFlight = false;

        function fetchProfiles() {
            if (_fetchInFlight) return;
            _fetchInFlight = true;

            loader.style.display = '';
            empty.style.display  = 'none';
            list.style.display   = 'none';
            list.innerHTML       = '';

            fetch('/api/face_profiles')
                .then(r => r.json())
                .then(res => {
                    loader.style.display = 'none';
                    const raw = res.profiles || [];

                    const seen = new Set();
                    const profiles = raw.filter(p => {
                        if (seen.has(p.face_id)) return false;
                        seen.add(p.face_id);
                        return true;
                    });

                    countBadge.textContent = profiles.length;
                    countBadge.classList.toggle('has-count', profiles.length > 0);

                    if (!profiles.length) {
                        empty.style.display = '';
                        return;
                    }

                    list.innerHTML = '';
                    profiles.forEach(p => {
                        const initials = (p.name || '?')
                            .split(/\s+/).map(w => w[0]).join('').substring(0, 2).toUpperCase();
                        const isActive = _activeFaceIds.has(p.face_id);
                        const card = document.createElement('div');
                        card.className = 'rp-profile-card' + (isActive ? ' rp-profile-active' : '');
                        card.dataset.faceId = p.face_id;
                        card.innerHTML = `
                            <span class="rp-profile-indicator${isActive ? ' active' : ''}"></span>
                            <div class="rp-profile-avatar">${initials}</div>
                            <div class="rp-profile-info">
                                <div class="rp-profile-name">${p.name || 'Unnamed'}</div>
                                <div class="rp-profile-id">ID: ${p.face_id}</div>
                            </div>`;
                        list.appendChild(card);
                    });
                    list.style.display = '';
                })
                .catch(() => {
                    loader.style.display = 'none';
                    empty.style.display  = '';
                })
                .finally(() => { _fetchInFlight = false; });
        }

        window._refreshProfileCards = function(activeIds) {
            _activeFaceIds = new Set(activeIds);
            document.querySelectorAll('.rp-profile-card').forEach(card => {
                const fid = Number(card.dataset.faceId);
                const isActive = _activeFaceIds.has(fid);
                card.classList.toggle('rp-profile-active', isActive);
                const dot = card.querySelector('.rp-profile-indicator');
                if (dot) dot.classList.toggle('active', isActive);
            });
        };

        toggle.addEventListener('click', () => {
            const isOpen = toggle.classList.toggle('open');
            dropdown.classList.toggle('open', isOpen);
            if (isOpen && !loaded) {
                loaded = true;
                fetchProfiles();
            }
        });

        const origOkBtn = document.getElementById('fid-ok-btn');
        if (origOkBtn) {
            origOkBtn.addEventListener('click', () => {
                setTimeout(() => {
                    if (toggle.classList.contains('open')) {
                        fetchProfiles();
                    } else {
                        loaded = false;
                    }
                }, 1500);
            });
        }
    })();

    
    
    // ========================================================================
    // SECTION 6: TAB NAVIGATION & LAYOUT MANAGEMENT
    // ========================================================================
    
    /**
     * Activates the live-streaming tab panel and starts detection polling.
     */
    function switchToLiveStreaming() {
        tabPanels.forEach(panel => panel.classList.remove('active'));
        const activePanel = document.getElementById('live-streaming-panel');
        if (activePanel) activePanel.classList.add('active');
        dashboardContainer.classList.add('live-streaming-active');
        startDataUpdates();
    }
    
    /**
     * Info Modal - displays application information from config.json
     */
    const infoModal = document.getElementById('info-modal');
    const infoModalBody = document.getElementById('info-modal-body');
    const infoModalClose = document.getElementById('info-modal-close');
    const infoModalOk = document.getElementById('info-modal-ok');
    
    /**
     * Populates and shows the info modal with data from CONFIG.info
     */
    function showInfoModal() {
        if (!infoModal || !infoModalBody) {
            console.error('Info modal elements not found');
            return;
        }
        
        // Get info from CONFIG (loaded from /api/config at page load)
        const infoData = CONFIG.info || {};
        console.log('CONFIG object:', CONFIG);
        console.log('CONFIG.info:', infoData);
        
        // Build HTML for info items
        let html = '';
        for (const [key, value] of Object.entries(infoData)) {
            html += `
                <div class="info-item">
                    <span class="info-item-key">${key}</span>
                    <span class="info-item-value">${value}</span>
                </div>
            `;
        }
        html += `
            <div class="info-modal-footer">
                © ${new Date().getFullYear()} Lattice Semiconductor
            </div>
        `;
        
        infoModalBody.innerHTML = html;
        infoModal.classList.remove('hidden');
    }
    
    /**
     * Info button click handler - show information modal popup
     */
    if (infoBtn) {
        infoBtn.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            showInfoModal();
        });
    }
    
    // Info modal close handlers
    if (infoModalClose) {
        infoModalClose.addEventListener('click', () => {
            infoModal.classList.add('hidden');
        });
    }
    if (infoModalOk) {
        infoModalOk.addEventListener('click', () => {
            infoModal.classList.add('hidden');
        });
    }
    // Close on backdrop click
    if (infoModal) {
        infoModal.querySelector('.modal-backdrop')?.addEventListener('click', () => {
            infoModal.classList.add('hidden');
        });
    }
    
    
    // ========================================================================
    // SECTION 8: MODAL DIALOGS
    // ========================================================================

    // Circuit viewer modal
    const circuitViewer = document.getElementById('circuit-viewer');
    if (circuitViewer) {
        const circuitViewBtn = document.getElementById('circuit-view-btn');
        const closeCircuitBtn = document.getElementById('close-circuit-btn');
        
        if (circuitViewBtn) {
            circuitViewBtn.addEventListener('click', () => circuitViewer.classList.remove('hidden'));
        }
        if (closeCircuitBtn) {
            closeCircuitBtn.addEventListener('click', () => circuitViewer.classList.add('hidden'));
        }
    }
    
    // ========================================================================
    // SECTION 9: CAMERA DISPLAY INIT (titles + resolutions for live streaming)
    // ========================================================================


    
    
    if (CONFIG.cameras) {
        const clnxTitle = document.getElementById('clnx-camera-title');
        if (clnxTitle && CONFIG.cameras.usb) {
            clnxTitle.textContent = CONFIG.cameras.usb.display_name || 'Live Video Feed through CLNX USB Camera';
        }
    }
    
    
    
    
    
    // ========================================================================
    // SECTION 10: UTILITY FUNCTIONS
    // ========================================================================
    
    /**
     * Displays a toast notification message to the user.
     * 
     * This function displays a temporary notification message (toast) to inform
     * the user of various events such as success, errors, warnings, or info messages.
     * It first checks if a global showToast function exists (which may be provided
     * by a toast library), and if not, it creates a simple DOM-based toast
     * implementation. The toast appears at the bottom of the screen, auto-dismisses
     * after a few seconds, and supports different types (success, error, warning, info)
     * with corresponding colors and icons.
     * 
     * @param {string} message - The message text to display
     * @param {string} type - Type of toast: 'success', 'error', 'warning', or 'info' (default: 'info')
     */
    function showToast(message, type = 'info') {
        if (typeof window.showToast === 'function') {
            window.showToast(message, type);
        } else {
            const toast = document.createElement('div');
            toast.className = `toast-notification toast-${type}`;
            
            const iconMap = {
                success: 'check-circle',
                error: 'exclamation-circle',
                warning: 'exclamation-triangle',
                info: 'info-circle'
            };
            
            toast.innerHTML = `
                <i class="fas fa-${iconMap[type] || 'info-circle'}"></i>
                <span>${message}</span>
            `;
            document.body.appendChild(toast);
            
            const showDelay = CONFIG.delays?.toast_show_delay || 10;
            const hideDelay = CONFIG.delays?.toast_hide_delay || 3000;
            const removeDelay = CONFIG.delays?.toast_remove_delay || 300;
            
            setTimeout(() => toast.classList.add('show'), showDelay);
            setTimeout(() => {
                toast.classList.remove('show');
                setTimeout(() => toast.remove(), removeDelay);
            }, hideDelay);
        }
    }
    
    // ========================================================================
    // SECTION 11: INITIALIZATION
    // ========================================================================

    // Go directly to live streaming dashboard on load
    const loadingOverlay = document.getElementById('loading-overlay');
    if (loadingOverlay) {
        loadingOverlay.classList.add('active');
        const loadingText = loadingOverlay.querySelector('.loading-text');
        if (loadingText) loadingText.textContent = 'Loading Katana Live Streaming Dashboard...';
        setTimeout(() => {
            switchToLiveStreaming();
            loadingOverlay.classList.remove('active');
        }, 1500);
    } else {
        switchToLiveStreaming();
    }
});

