# LENS Engine Structure

## Flow

```
main()
  └── Engine::start()
        ├── Database::open()                    // open SQLite, create tables
        ├── load cameras from DB
        ├── CameraManager::addCamera() x N      // one worker thread per camera
        ├── CameraManager::startAll()           // workers begin reading frames
        │     └── per camera, in a loop:
        │           CameraCapture::readFrame()
        │             └── FrameBuffer::push(frame)
        │                   └── when buffer is full:
        │                         VideoMAE::run(buffer) → NORMAL / SUSPICIOUS
        │                               └── if SUSPICIOUS:
        │                                     IncidentPipeline::handle(cameraId, frames)
        │                                       ├── ClipRecorder::write()  → clipPath
        │                                       ├── IncidentStore::save()  → synced=0
        │                                       ├── R2Uploader::upload()
        │                                       ├── BackendClient::send()
        │                                       └── IncidentStore::markSynced()
        ├── SyncWorker::start()                 // background thread, retries synced=0
        └── HttpServer::start()                 // localhost, receives commands from Interface
```

---

## Classes

### Already Exists (Camera Layer)

| Class | Job |
|---|---|
| `CameraConfig` | Plain struct for camera data |
| `CameraCapture` | Opens stream, reads one frame |
| `CameraWorker` | Background thread per camera |
| `CameraManager` | Owns all workers, starts/stops them |
| `CameraStore` | SQLite `cameras` table |

### Database Layer

| Class | Job |
|---|---|
| `IncidentStore` | SQLite `incidents` table — save, markSynced, getUnsynced |
| `ConfigStore` | SQLite `config` table — get/set key-value pairs (API key lives here) |

### AI Pipeline

| Class | Job |
|---|---|
| `FrameBuffer` | Accumulates N sampled frames per camera; signals when full |
| `VideoMAE` | Wraps ONNX Runtime session; takes a full buffer, returns `NORMAL` or `SUSPICIOUS` |

### Incident Pipeline

| Class | Job |
|---|---|
| `ClipRecorder` | Writes frames to an MP4 on disk using OpenCV `VideoWriter` |
| `R2Uploader` | HTTP PUT to Cloudflare R2 (libcurl) |
| `BackendClient` | HTTP POST to backend with incident metadata + `X-API-Key` header |
| `IncidentPipeline` | Orchestrates the four steps above in order |

### Retry

| Class | Job |
|---|---|
| `SyncWorker` | Background thread; polls `IncidentStore::getUnsynced()` and retries upload + send |

### HTTP Server

| Class | Job |
|---|---|
| `HttpServer` | Listens on localhost; routes: `POST /cameras`, `DELETE /cameras/:id`, `POST /config/apikey` |

### Top Level

| Class | Job |
|---|---|
| `Engine` | Owns everything, wires it all together, starts/stops cleanly |

---

## Current Files

### `CameraConfig.h`
A plain data struct — just holds the fields for one camera: `id`, `name`, `url`, `password`, `location`. No logic, no methods. Everything else uses this struct to pass camera data around.

### `CameraCapture.h/.cpp`
Wraps OpenCV's `VideoCapture`. Knows how to open a stream (RTSP, HTTP, or local file), read one frame into a `cv::Mat`, and close the connection. Also handles one special case: if the source is a local file (not a network stream), it loops back to frame 0 when the file ends — useful for testing with MP4s.

### `CameraWorker.h/.cpp`
One instance per camera. Runs a loop on its own background thread: open the camera, read frames continuously, and when the connection drops it closes and retries after 8 seconds. Has a `// TODO` stub where the frame goes — that's where `FrameBuffer` will plug in. Owns a `CameraCapture` internally.

### `CameraManager.h/.cpp`
Owns a list of `CameraWorker`s. `addCamera()` creates a new worker, `startAll()` launches all their threads, `stopAll()` shuts them all down cleanly. Forwards error callbacks from workers up to Engine.

### `CameraStore.h/.cpp`
SQLite persistence for the `cameras` table. Four operations: `open()` creates the DB and table if they don't exist, `save()` inserts a camera and returns its new ID, `remove()` deletes by ID, `loadAll()` returns every camera. Uses the raw SQLite3 C API.

### `main.cpp`
Entry point only. Sets up signal handlers for `SIGINT`/`SIGTERM`, spins in a loop until a signal fires. Has a `// TODO` for starting the HTTP server and AI pipeline. Intentionally thin — all real logic goes in `Engine`.

### `CMakeLists.txt`
Build configuration. Compiles with C++17, finds OpenCV and SQLite3, lists all source files, and links the dependencies.

### `run.sh`
Build and run script. Wipes the old build folder, re-configures with CMake and Ninja, builds, then runs the binary. Used during development to do a clean build and launch in one command.

---

## FrameBuffer Design Notes

VideoMAE is a video model — it needs a clip, not a single frame. Decisions to make:

- **How many frames:** VideoMAE typically uses 16 frames
- **Sample rate:** e.g. 1 frame every 500ms → 16 frames = 8 seconds of context
- **Window strategy:** fixed window (fill 16, run, clear, repeat) is simplest to start with

The `FrameBuffer` lives inside `CameraWorker` and signals the worker when enough frames have been collected to trigger inference.

---

## File Layout

```
engine/
  includes/
    camera/
      CameraConfig.h
      CameraCapture.h
      CameraWorker.h
      CameraManager.h
    ai/
      FrameBuffer.h
      VideoMAE.h
    incident/
      ClipRecorder.h
      R2Uploader.h
      BackendClient.h
      IncidentPipeline.h
    store/
      CameraStore.h
      IncidentStore.h
      ConfigStore.h
    sync/
      SyncWorker.h
    http/
      HttpServer.h
    Engine.h
  src/
    camera/
      CameraCapture.cpp
      CameraWorker.cpp
      CameraManager.cpp
      CameraStore.cpp
    ai/
      FrameBuffer.cpp
      VideoMAE.cpp
    incident/
      ClipRecorder.cpp
      R2Uploader.cpp
      BackendClient.cpp
      IncidentPipeline.cpp
    store/
      IncidentStore.cpp
      ConfigStore.cpp
    sync/
      SyncWorker.cpp
    http/
      HttpServer.cpp
    Engine.cpp
    main.cpp
```

---

## Separation of Concerns

### main.cpp is nearly empty

`main.cpp` has one job — entry point. Signal handling, create Engine, start it, wait. No business logic ever lives here.

```
main.cpp
  → creates Engine
  → calls engine.start()
  → waits for shutdown signal
  → calls engine.stop()
```

### Engine::start() is where startup logic lives

Loading cameras from SQLite and starting the HTTP server both happen inside `Engine::start()`, in this order:

```
Engine::start()
  1. open SQLite DB
  2. load all cameras from CameraStore
  3. for each camera → CameraManager::addCamera()
  4. CameraManager::startAll()         ← worker threads running
  5. HttpServer::start()               ← server listening
```

### HttpServer does not contain business logic

When the HTTP server receives `POST /cameras`, it does not handle the logic itself. It calls a callback that Engine registered at startup. Engine owns the logic.

```
HttpServer receives POST /cameras
  → calls callback registered by Engine
    → Engine::addCamera(config)
      → CameraStore::save(config)
      → CameraManager::addCamera(config)    ← worker thread starts immediately
```

### File responsibility summary

| File | Responsibility |
|---|---|
| `main.cpp` | Entry point only. Create Engine, start, wait, stop. |
| `Engine.h/.cpp` | Owns all subsystems. Wires callbacks. Startup/shutdown order. |
| `HttpServer.h/.cpp` | Parses HTTP requests, calls registered callbacks. No business logic. |
| `CameraManager.h/.cpp` | Manages worker threads. No knowledge of HTTP or DB. |
| `CameraStore.h/.cpp` | SQLite only. No knowledge of workers or HTTP. |

Each class knows only what it needs to. Engine is the only class that knows about all the others.

---

## Class Functions

### Already Built

#### `CameraConfig`
No functions — plain data struct.
```
id, name, url, password, location
```

#### `CameraCapture`
```
open()          → opens the stream with OpenCV
readFrame()     → reads one frame into a cv::Mat
close()         → releases the stream
isFileSource()  → private, checks if url is a file vs network stream
```

#### `CameraWorker`
```
start()              → launches the background thread
stop()               → sets running=false, joins thread, closes camera
setErrorCallback()   → registers the error handler from Engine
run()                → private, the thread loop: open → read frames → retry on failure
```

#### `CameraManager`
```
addCamera()          → creates a CameraWorker, registers error callback, adds to list
startAll()           → calls start() on every worker
stopAll()            → calls stop() on every worker, clears the list
setErrorCallback()   → stores the callback, forwards it to each worker
```

#### `CameraStore`
```
open()      → opens SQLite DB, creates cameras table if it doesn't exist
save()      → inserts a camera, returns the new ID
remove()    → deletes a camera by ID
loadAll()   → returns every camera as a vector of CameraConfig
```

---

### Still To Build

#### `IncidentStore`
```
open()          → creates incidents table if it doesn't exist
save()          → inserts an incident with synced=0, returns new ID
markSynced()    → sets synced=1 for a given incident ID
getUnsynced()   → returns all incidents where synced=0
```

#### `ConfigStore`
```
open()   → creates config table if it doesn't exist
set()    → inserts or updates a key-value pair
get()    → returns the value for a given key
```

#### `FrameBuffer`
```
push()       → adds a frame to the buffer, sampled at the configured rate
isFull()     → returns true when 16 frames have been collected
getFrames()  → returns the collected frames for inference
clear()      → empties the buffer after inference runs
```

#### `VideoMAE`
```
load()   → loads the .onnx model file into an ONNX Runtime session
run()    → takes the frame buffer, runs inference, returns NORMAL or SUSPICIOUS
```

#### `ClipRecorder`
```
write()   → takes a vector of frames, writes them to an MP4 on disk using OpenCV VideoWriter, returns the file path
```

#### `R2Uploader`
```
upload()   → takes a local file path, HTTP PUTs it to Cloudflare R2, returns success/fail
```

#### `BackendClient`
```
sendIncident()   → HTTP POST to backend with incident metadata and X-API-Key header, returns success/fail
```

#### `IncidentPipeline`
```
handle()   → orchestrates the full incident flow:
               1. ClipRecorder::write()
               2. IncidentStore::save()
               3. R2Uploader::upload()
               4. BackendClient::sendIncident()
               5. IncidentStore::markSynced()
```

#### `SyncWorker`
```
start()   → launches background thread
stop()    → stops the thread
run()     → private, thread loop: calls IncidentStore::getUnsynced(), retries
              R2Uploader and BackendClient for each, marks synced on success
```

#### `HttpServer`
```
setAddCameraCallback()      → registers the function Engine passes in
setRemoveCameraCallback()   → registers the function Engine passes in
start()                     → registers routes, calls listen() on a background thread
stop()                      → stops the server
```
Routes defined inside `start()`:
```
POST /cameras        → parse JSON body into CameraConfig, call addCamera callback
DELETE /cameras/:id  → parse id, call removeCamera callback
POST /config/apikey  → parse key from body, call setApiKey callback
```

#### `Engine`
```
start()          → open DB, load cameras, start CameraManager, start HttpServer
stop()           → stop HttpServer, stop CameraManager, stop SyncWorker
addCamera()      → CameraStore::save() + CameraManager::addCamera()
removeCamera()   → CameraStore::remove() + stop that worker in CameraManager
setApiKey()      → ConfigStore::set("apiKey", value)
```

---

## Suggested Build Order

1. `IncidentStore` + `ConfigStore` — almost everything else depends on the DB being complete
2. `FrameBuffer` + `VideoMAE` — the core detection loop
3. `ClipRecorder` + `R2Uploader` + `BackendClient` + `IncidentPipeline` — incident handling
4. `HttpServer` — Interface communication
5. `Engine` — wire everything together
6. `SyncWorker` — retry logic, can come last
