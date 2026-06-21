# LENS

**Loss-prevention Event Notification System**

LENS is an AI-powered retail loss-prevention system that uses existing security camera feeds to detect suspicious concealment events and notify store staff with a short video clip for review.

Instead of replacing a store's current DVR/NVR system, LENS runs alongside it. The existing security system continues handling live viewing and recording, while LENS focuses on event detection, incident clips, and mobile alerts.

---

## Overview

LENS watches authorized camera feeds in real time and detects suspicious actions such as objects being placed into pockets, bags, or clothing. When an event is detected it saves a short video clip, uploads it to cloud storage, and sends a push notification to the store's mobile app.

```text
Camera Feed
    ↓
LENS Engine (AI Inference)
    ↓
Suspicious Event Detected
    ↓
Clip Uploaded to Cloudflare R2
    ↓
Incident Stored in Cloud DB
    ↓
Push Notification → Mobile App
    ↓
Staff Reviews Clip
```

---

## Architecture

LENS is made up of five parts:

### 1. Store Server (On-Premise)

A machine located inside the retail store running two applications.

#### LENS Engine (`engine/`)

The core on-site process written in C++. Responsible for:

- Connecting to RTSP/HTTP camera streams
- Reading frames and maintaining a rolling buffer per camera
- Running the VideoMAE AI model every 5 frames to detect concealment
- On detection: encoding a clip, uploading to Cloudflare R2, notifying the backend
- Exposing a local REST API on `127.0.0.1:7373` for the Interface to talk to

The engine authenticates with the backend using a **store API key** — generated at account creation, stored in the engine's local SQLite database, and sent as `X-API-Key` on every outbound request.

#### LENS Interface (`interface/`)

A Qt (C++) desktop application used by store staff to:

- Create the store account (links the server ID to an email + password)
- Add and remove cameras
- View live camera connection status
- Monitor the system

The Interface communicates with the Engine over localhost and with the Backend over HTTPS. It never communicates with R2 directly.

```text
Interface ──→ localhost:7373 ──→ Engine
Interface ──→ HTTPS ──→ Backend
```

#### Local SQLite Database (Engine)

```sql
cameras  (id, name, url, password, location)
config   (key, value)   -- api_key, backend_url
```

---

### 2. Cloud Backend (`backend/`)

Node.js / Express API. Responsible for:

- User registration and login (JWT)
- Server ID validation (each physical server has a pre-registered unique ID)
- Store account creation
- Engine authentication (`X-API-Key` → store lookup)
- Generating presigned R2 upload URLs for the engine
- Storing incident metadata in MySQL
- Sending Expo push notifications to registered mobile devices
- Serving incident data and presigned R2 clip URLs to the mobile app

The backend never communicates directly with the Store Server — all communication is outbound from the engine or mobile app.

---

### 3. Cloud MySQL Database

```sql
servers    (id, in_use, store_id)               -- pre-registered server IDs
stores     (id, store_name, store_number, api_key)
users      (id, store_id, email, password_hash)
incidents  (id, store_id, camera_id, timestamp, clip_path, incident_type, reviewed)
push_tokens (id, user_id, token)
```

Camera configuration is stored locally on the engine only and is not synced to the cloud.

---

### 4. Cloudflare R2 (Video Storage)

Incident clips are stored at `clips/{store_id}/{timestamp}_cam{cameraId}.mp4`.

- The engine requests a presigned PUT URL from the backend, then uploads directly to R2
- The mobile app never receives a raw R2 URL — the backend generates a short-lived presigned GET URL (5 min TTL) on demand
- Video never passes through the backend

---

### 5. Mobile App (`mobile/`)

React Native (Expo SDK 54) app. Responsible for:

- User login
- Receiving push notifications when an incident is detected
- Viewing the incident list
- Streaming incident clips via presigned R2 URLs
- Marking incidents as reviewed

The app communicates only with the Cloud Backend.

---

## Key Flows

### Account Creation

```text
1. Staff opens LENS Interface, clicks "Create Account"
2. Enters: Server ID, email, password, store name, store number
3. Interface POSTs to backend /auth/register
4. Backend validates the Server ID exists and is not already in use
5. Backend creates store + user records, generates a unique engine API key
6. Backend marks the server as in_use
7. Interface POSTs the API key to the engine at localhost:7373/config/setup
8. Engine saves api_key and backend_url to its local SQLite config table
```

### Camera Configuration

```text
1. Staff opens LENS Interface, clicks "Add Camera"
2. Enters: name, stream URL, password, location
3. Interface POSTs to engine at localhost:7373/cameras
4. Engine saves camera to SQLite and starts a worker thread immediately
5. Worker thread connects to the RTSP stream and begins reading frames
```

### Incident Detection

```text
1. CameraWorker reads frames into a rolling 5-second buffer
2. Every 5 frames: Inferencer runs VideoMAE on 16 evenly-sampled frames
3. If P(suspicious) ≥ 0.5 and 30-second cooldown has elapsed:
4.   IncidentHandler encodes the buffer to a local MP4
5.   Engine requests presigned R2 PUT URL from backend (X-API-Key auth)
6.   Engine uploads clip directly to R2
7.   Engine POSTs incident metadata to backend
8.   Backend stores incident in MySQL
9.   Backend sends Expo push notification to all registered devices for that store
10.  Staff receives notification and opens clip in the mobile app
```

### Incident Viewing

```text
1. Staff opens mobile app (or taps notification)
2. App fetches incident list from backend (JWT auth)
3. Staff taps an incident
4. App requests clip URL from backend
5. Backend generates a presigned R2 GET URL (5 min TTL)
6. App streams clip directly from R2
```

---

## AI Model

The engine runs a fine-tuned **VideoMAE** model (exported to ONNX) for binary video classification.

```text
Input:  16 frames sampled from the camera buffer
        resized to 224×224, ImageNet-normalised
        tensor shape: [1, 16, 3, 224, 224]

Output: logits [normal, suspicious]
        softmax → P(suspicious)
        threshold: 0.5
```

Model weights: ~334 MB (stored separately as `model.onnx.data`).

---

## Technology Stack

| Component | Technology |
|---|---|
| Engine | C++17, OpenCV, ONNX Runtime, SQLite3, libcurl, cpp-httplib |
| Interface | Qt6 (C++), QNetworkAccessManager |
| Backend | Node.js, Express, MySQL2, JWT, AWS SDK (R2), bcrypt |
| Mobile App | React Native, Expo SDK 54, expo-notifications, expo-video |
| Database | MySQL (cloud) + SQLite (local engine) |
| Video Storage | Cloudflare R2 |
| Push Notifications | Expo Push Notifications API |
| Build System | CMake (engine), EAS Build (mobile) |

---

## Project Structure

```text
LENS/
├── backend/
│   ├── api/
│   │   ├── auth.js               public auth routes (register, login)
│   │   ├── engine.js             engine-authenticated routes (incidents)
│   │   └── loggedIn.js           JWT-protected routes (incidents, push-token)
│   ├── controllers/
│   │   ├── authController.js     register, login, savePushToken
│   │   ├── engineController.js   requestUpload, createIncident
│   │   └── incidentController.js getIncidents, getClipUrl, markReviewed
│   ├── db/
│   │   ├── connection.js         MySQL pool
│   │   └── migrate.sql           schema migrations
│   ├── middleware/
│   │   ├── auth.js               JWT verification
│   │   └── engineAuth.js         X-API-Key → store_id lookup
│   ├── app.js                    Express app entry point
│   └── run.sh
│
├── engine/
│   ├── includes/
│   │   ├── camera/
│   │   │   ├── CameraCapture.h
│   │   │   ├── CameraConfig.h
│   │   │   ├── CameraManager.h
│   │   │   ├── CameraStore.h
│   │   │   └── CameraWorker.h
│   │   ├── engine/
│   │   │   └── Engine.h
│   │   ├── http/
│   │   │   ├── httplib.h
│   │   │   └── HttpServer.h
│   │   ├── incident/
│   │   │   └── IncidentHandler.h
│   │   └── inference/
│   │       └── Inferencer.h
│   ├── src/
│   │   ├── camera/
│   │   │   ├── CameraCapture.cpp
│   │   │   ├── CameraManager.cpp
│   │   │   ├── CameraStore.cpp
│   │   │   └── CameraWorker.cpp
│   │   ├── engine/
│   │   │   └── Engine.cpp
│   │   ├── http/
│   │   │   └── HttpServer.cpp
│   │   ├── incident/
│   │   │   └── IncidentHandler.cpp
│   │   ├── inference/
│   │   │   └── Inferencer.cpp
│   │   └── main.cpp
│   ├── models/
│   │   ├── model.onnx            VideoMAE model (symlink to production model)
│   │   ├── model.onnx.data       model weights — 334MB (symlink)
│   │   └── test.mp4              test video for /test/video endpoint
│   ├── database/
│   │   └── lens.db               SQLite database (cameras + config)
│   ├── CMakeLists.txt
│   ├── run.sh                    build with inference + launch
│   └── test_video.sh             run test.mp4 through the full pipeline
│
├── interface/
│   ├── AddCameraDialog.h/.cpp    dialog for adding cameras
│   ├── ApiClient.h/.cpp          HTTP client (engine + backend)
│   ├── CameraCard.h/.cpp         camera status widget
│   ├── MainWindow.h/.cpp         main application window
│   ├── SetupDialog.h/.cpp        account creation dialog
│   ├── main.cpp
│   ├── resources.qrc
│   ├── CMakeLists.txt
│   └── run.sh
│
├── mobile/
│   ├── screens/
│   │   ├── HomeScreen.js
│   │   ├── IncidentsScreen.js
│   │   ├── IncidentDetailScreen.js
│   │   └── LoginScreen.js
│   ├── context/
│   │   ├── AuthContext.js        JWT storage + login/logout
│   │   └── NotificationContext.js incident list + push handling
│   ├── components/
│   │   └── NotificationBell.js
│   ├── utils/
│   │   └── api.js                authFetch helper
│   ├── assets/
│   │   └── lens_logo.jpg
│   ├── App.js                    navigation + push token registration
│   ├── app.json                  Expo config (projectId, plugins)
│   ├── eas.json                  EAS build profiles
│   └── run.sh                    npx expo start --dev-client
│
└── docs/
    ├── README.md                 this file
    ├── ARCHITECTURE.md
    ├── ENGINE_STRUCTURE.md
    └── LENS.drawio
```

---

## Running Locally

### Backend
```bash
cd backend && ./run.sh
```

### Engine
```bash
cd engine && ./run.sh
```
Builds with `ENABLE_INFERENCE=ON` and starts with the ONNX Runtime library path set automatically.

### Interface
```bash
cd interface && ./run.sh
```

### Mobile (development)
```bash
cd mobile && ./run.sh
```
Starts the Metro bundler. Open the LENS dev client app on your phone and scan the QR code.

### Test the pipeline
```bash
cd engine && ./test_video.sh
```
Runs `models/test.mp4` through the full AI → R2 → push notification pipeline.
