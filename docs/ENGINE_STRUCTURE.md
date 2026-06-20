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

## Suggested Build Order

1. `IncidentStore` + `ConfigStore` — almost everything else depends on the DB being complete
2. `FrameBuffer` + `VideoMAE` — the core detection loop
3. `ClipRecorder` + `R2Uploader` + `BackendClient` + `IncidentPipeline` — incident handling
4. `HttpServer` — Interface communication
5. `Engine` — wire everything together
6. `SyncWorker` — retry logic, can come last
