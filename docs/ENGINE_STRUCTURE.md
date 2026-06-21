# LENS Engine — Structure & Flow

## Overview

The engine is a C++17 process that runs on the same machine as the security cameras. It has three jobs:

1. Read frames from RTSP/HTTP camera streams
2. Run AI inference to detect suspicious behaviour
3. When detected — record a clip, upload it to cloud storage, notify the backend

It exposes a local REST API on `127.0.0.1:7373` so the Qt desktop interface can manage cameras and configuration without touching the engine internals directly.

---

## File Layout

```
engine/
  includes/
    camera/
      CameraConfig.h          plain data struct
      CameraCapture.h         OpenCV stream wrapper
      CameraWorker.h          per-camera background thread
      CameraManager.h         owns all workers
      CameraStore.h           SQLite (cameras + config tables)
    inference/
      Inferencer.h            VideoMAE ONNX model wrapper
    incident/
      IncidentHandler.h       full incident pipeline
    http/
      HttpServer.h            cpp-httplib REST API
      httplib.h               cpp-httplib single-header library
    engine/
      Engine.h                top-level orchestrator
  src/
    camera/
      CameraCapture.cpp
      CameraWorker.cpp
      CameraManager.cpp
      CameraStore.cpp
    inference/
      Inferencer.cpp          (compiled only when ENABLE_INFERENCE=ON)
    incident/
      IncidentHandler.cpp
    http/
      HttpServer.cpp
    engine/
      Engine.cpp
    main.cpp
  models/
    model.onnx                VideoMAE model (symlink to production model)
    model.onnx.data           model weights — 334MB (symlink)
    test.mp4                  test video for the /test/video endpoint
  database/
    lens.db                   SQLite database (cameras + config)
  CMakeLists.txt
```

---

## Classes

### `CameraConfig`
Plain data struct. No methods. Passed around everywhere to describe a camera.

```
id        int     — assigned by SQLite on save
name      string  — human label, e.g. "Aisle 5"
url       string  — RTSP or HTTP stream address
password  string  — stream auth password
location  string  — physical location label
```

---

### `CameraCapture`
Wraps OpenCV `VideoCapture`. Knows how to open a stream, read one frame, and close it. If the source URL is a local file (for testing), it automatically loops back to frame 0 when the file ends.

```
open()        opens the stream
readFrame()   reads one frame into a cv::Mat, returns false on failure
close()       releases the stream
getFps()      returns the stream's reported frame rate
```

---

### `CameraWorker`
One instance per camera. Runs its own background thread. This is where frames are read and inference is triggered.

**Thread loop:**
1. Call `CameraCapture::open()` — if it fails, wait 8 seconds and retry
2. Set `m_connected = true`
3. Read frames in a loop, pushing each into a rolling deque buffer (5 seconds of frames)
4. Every 5 frames: call `Inferencer::classify()` on the current buffer contents
5. If the model returns suspicious and the cooldown has expired (30s): copy the buffer, fire `IncidentHandler::trigger()`
6. If `readFrame()` fails: set `m_connected = false`, close, break back to step 1

**Members:**
```
m_backendUrl      where to reach the backend (set from SQLite config at startup)
m_apiKey          engine's credential (set from SQLite config at startup)
m_connected       atomic bool — polled by HttpServer for /cameras status
m_frameBuffer     rolling deque, max 5 seconds of frames
m_inferencer      VideoMAE model instance (ENABLE_INFERENCE only)
m_incidentHandler pipeline instance (ENABLE_INFERENCE only)
m_lastIncident    timestamp of last triggered incident (for cooldown)
```

```
start()              launches the background thread
stop()               signals thread to exit, joins it
isConnected()        returns m_connected
setErrorCallback()   registers error handler (called on stream failure)
```

---

### `CameraManager`
Owns the list of all `CameraWorker` instances. The only class allowed to create or destroy workers.

```
addCamera(config)          creates a worker, passes it m_backendUrl + m_apiKey, registers error callback
removeCamera(id)           stops the worker for that camera ID and removes it
startAll()                 calls start() on every worker
stopAll()                  calls stop() on every worker
setConfig(url, key)        stores backend URL + API key, passes them to every existing worker
isConnected(cameraId)      asks the matching worker for its connection state
```

---

### `CameraStore`
SQLite persistence. Manages two tables in `database/lens.db`.

**cameras table** — one row per camera

```
save(config)    inserts, returns the new auto-incremented ID
remove(id)      deletes the camera row
loadAll()       returns every camera as a vector<CameraConfig>
```

**config table** — key/value pairs (uses `ON CONFLICT DO UPDATE` for upsert)

```
getConfig(key)         returns the value, empty string if not set
setConfig(key, value)  upserts the key/value pair
```

Current config keys: `api_key`, `backend_url`

---

### `Inferencer`
Wraps an ONNX Runtime session running the VideoMAE production model. Only compiled when `ENABLE_INFERENCE=ON` in CMake.

**Model spec:**
- Input: `pixel_values` — shape `[1, 16, 3, 224, 224]`
- Output: `logits` — shape `[1, 2]` (index 0 = normal, index 1 = suspicious)

**`classify(frames, threshold=0.5f)`**
1. Samples 16 frames evenly from however many were passed in
2. Resizes each to 224×224, converts BGR→RGB
3. Normalises with ImageNet mean `[0.485, 0.456, 0.406]` and std `[0.229, 0.224, 0.225]`
4. Arranges into `[1, 16, 3, 224, 224]` tensor in CHW order
5. Runs the ONNX session
6. Applies softmax to the two logits
7. Returns `true` if P(suspicious) ≥ threshold

**`load()`** — loads the model file, logs the ORT exception if it fails.

Model path is resolved relative to the engine binary (`../models/model.onnx` from the `build/` directory) so it works regardless of what directory the process is launched from.

---

### `IncidentHandler`
Orchestrates the full incident pipeline. Each instance is tied to one `backendUrl` + `apiKey` pair. Runs asynchronously on a detached thread.

**`triggerShared(shared_ptr<self>, frames, fps, cameraId, incidentType)`**
Static method. Captures the `shared_ptr` in the thread lambda so the object outlives the caller. Used by `CameraWorker` and the test endpoints.

**`trigger(frames, fps, cameraId, incidentType)`**
Instance method version — only safe when the caller controls the lifetime (not used from worker threads).

**`isUploading()`** — returns true while a pipeline run is in progress. Used by test endpoints to reject duplicate requests.

**Internal pipeline (runs on detached thread):**

```
process()
  ├── recordClip()        encode frames → /tmp/lens_{timestamp}.mp4
  ├── getPresignedUrl()   POST /engine/incidents/request-upload → { uploadUrl, clipPath }
  ├── uploadClip()        HTTP PUT clip file to R2 presigned URL
  ├── (delete /tmp file)
  └── reportIncident()    POST /engine/incidents → backend creates DB record + sends push
```

All HTTP calls use libcurl. The `X-API-Key` header is sent on every backend request.

---

### `HttpServer`
cpp-httplib server on `127.0.0.1:7373`. Only accessible from the local machine. The Qt interface talks to this.

**Routes:**

| Method | Path | What it does |
|---|---|---|
| GET | `/config/status` | Returns `{"configured": true/false}` based on whether `api_key` is in SQLite |
| POST | `/config/setup` | Saves `api_key` + `backend_url` to SQLite, calls `CameraManager::setConfig()` |
| GET | `/cameras` | Returns all cameras with their live `connected` status |
| POST | `/cameras` | Adds a camera to SQLite and starts its worker thread |
| DELETE | `/cameras/:id` | Stops the worker and removes the camera from SQLite |
| POST | `/test/trigger` | Generates 150 synthetic frames and runs the full incident pipeline |
| POST | `/test/video` | Loads a `.mp4` from a given path, runs VideoMAE inference, triggers pipeline if suspicious |

The `/test/video` endpoint requires `ENABLE_INFERENCE=ON`. Without it, it still triggers the pipeline using all frames from the video (treats the whole clip as the incident).

---

### `Engine`
Owns one of every major subsystem. Wires them together. Responsible for startup and shutdown order.

```
Engine()    constructs CameraStore, CameraManager, HttpServer
start()     open DB → load config → set config on manager → load cameras → startAll → start HTTP
stop()      stop HTTP → stopAll cameras
```

---

### `main.cpp`
Entry point only. Installs `SIGINT`/`SIGTERM` handlers, creates `Engine`, calls `start()`, blocks until signal, calls `stop()`.

---

## Startup Sequence

```
main()
  └── Engine::start()
        ├── CameraStore::open()               open lens.db, create tables if missing
        ├── CameraStore::getConfig("api_key")
        ├── CameraStore::getConfig("backend_url")
        ├── CameraManager::setConfig()        pass credentials to manager
        ├── CameraStore::loadAll()            load saved cameras
        ├── CameraManager::addCamera() × N    create one worker per camera
        ├── CameraManager::startAll()         launch all worker threads
        └── HttpServer::start()               start listening on 7373
```

---

## Detection Flow (live camera)

```
CameraWorker thread (per camera)
  │
  ├── CameraCapture::open()         connect to RTSP stream
  │     └── on failure: wait 8s, retry
  │
  └── loop:
        CameraCapture::readFrame()
          └── push frame into rolling 5-second deque buffer
              │
              └── every 5 frames:
                    Inferencer::classify(buffer)
                      ├── sample 16 frames evenly from buffer
                      ├── resize to 224×224, BGR→RGB, ImageNet normalise
                      ├── run VideoMAE ONNX session
                      └── softmax → P(suspicious)
                          │
                          └── if P ≥ 0.5 AND cooldown elapsed (30s):
                                record lastIncident timestamp
                                IncidentHandler::triggerShared(
                                  shared_ptr<handler>, buffer_copy, fps,
                                  cameraId, "concealment_detected"
                                )
                                  └── detached thread:
                                        recordClip()          → /tmp/lens_*.mp4
                                        getPresignedUrl()     → R2 signed URL
                                        uploadClip()          → PUT to R2
                                        delete /tmp file
                                        reportIncident()      → POST /engine/incidents
```

---

## Account Creation & Engine Configuration Chain

This is how a new server gets linked to a user account.

```
User opens Qt interface → clicks "Create Account"
  │
  ├── SetupDialog collects: serverId, email, password, storeName, storeNumber
  │
  ├── POST /auth/register  →  backend
  │     ├── SELECT from servers WHERE id = serverId
  │     │     └── 404 if not found, 409 if already in_use
  │     ├── SELECT from users WHERE email = email
  │     │     └── 409 if already registered
  │     ├── BEGIN TRANSACTION
  │     │     ├── INSERT INTO stores (id, store_name, store_number, api_key)
  │     │     │     └── api_key = crypto.randomBytes(32).hex()   ← unique engine credential
  │     │     ├── INSERT INTO users (id, store_id, email, password_hash)
  │     │     └── UPDATE servers SET in_use=1, store_id=<new store>
  │     └── COMMIT → return { jwt, user, engine_api_key }
  │
  └── POST /config/setup  →  engine (127.0.0.1:7373)
        body: { api_key: engine_api_key, backend_url: "http://..." }
        │
        ├── CameraStore::setConfig("api_key", ...)
        ├── CameraStore::setConfig("backend_url", ...)
        └── CameraManager::setConfig(backendUrl, apiKey)
              └── passed to every new CameraWorker going forward
```

From this point the engine is configured. Every incident request it makes to the backend carries the `api_key` in `X-API-Key`, which the backend uses to look up `store_id` and scope all data (clip storage path, incident records, push notifications) to the correct account.

---

## Incident Reporting Chain (backend side)

When the engine calls `POST /engine/incidents/request-upload`:

```
engineAuth middleware
  └── SELECT store_id FROM stores WHERE api_key = ?
        └── attaches store_id to request

requestUpload controller
  └── generates R2 presigned PUT URL
        path: clips/{store_id}/{timestamp}_cam{cameraId}.mp4
        └── returns { uploadUrl, clipPath }
```

When the engine calls `POST /engine/incidents`:

```
engineAuth middleware
  └── looks up store_id from api_key

createIncident controller
  ├── INSERT INTO incidents (id, store_id, camera_id, timestamp, clip_path, incident_type)
  └── SELECT token FROM push_tokens WHERE user_id IN (SELECT id FROM users WHERE store_id = ?)
        └── POST to https://exp.host/--/api/v2/push/send
              └── push notification delivered to user's phone
```

---

## Build Configuration

```bash
# Without inference (default — no ONNX Runtime needed)
cmake -B build
cmake --build build -j$(nproc)

# With inference (requires ONNX Runtime)
cmake -B build \
  -DENABLE_INFERENCE=ON \
  -DORT_ROOT=/path/to/onnxruntime-linux-x64-1.26.0
cmake --build build -j$(nproc)
```

When `ENABLE_INFERENCE=OFF`:
- `Inferencer.cpp` is not compiled
- `CameraWorker` inference block is compiled out via `#ifdef LENS_INFERENCE_ENABLED`
- `HttpServer` `/test/video` still works but skips inference and treats the whole video as the clip

Runtime requires the ONNX Runtime shared library in `LD_LIBRARY_PATH`:
```bash
LD_LIBRARY_PATH=/path/to/onnxruntime/lib ./build/LENS
```

---

## Dependencies

| Library | Used for |
|---|---|
| OpenCV | Frame capture, resize, colour conversion, VideoWriter for clip encoding |
| SQLite3 | Local persistence (cameras table, config table) |
| libcurl | HTTP calls from IncidentHandler to backend and R2 |
| ONNX Runtime | Running the VideoMAE model (ENABLE_INFERENCE=ON only) |
| cpp-httplib | Single-header HTTP server for the local REST API |
