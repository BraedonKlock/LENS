# LENS

**Loss-prevention Event Notification System**

LENS is an AI-powered retail loss-prevention system that uses existing security camera feeds to detect suspicious concealment events and notify users with a short video clip for review.

Instead of replacing a store's current DVR/NVR system, LENS runs alongside it. The existing security system continues handling live viewing and recording, while LENS focuses on event detection, incident clips, and alerts.

---

## Overview

LENS watches authorized camera feeds in real time and detects suspicious actions such as objects being placed into pockets, bags, or clothing.

When an event is detected, LENS saves a short video clip and sends the incident to a React Native app where the user can review it.

```text
Camera Feed
    ↓
LENS Engine
    ↓
Suspicious Event Detected
    ↓
Incident Clip Saved
    ↓
User Receives Notification
    ↓
User Reviews Clip in Mobile App
```

---

## Architecture

LENS is made up of five main parts:

### 1. Store Server (On-Premise)

A physical server located inside the retail store running two separate applications.

#### LENS Engine

The core on-site process responsible for:

- Connecting to cameras
- Reading video frames
- Running AI inference
- Detecting suspicious activity
- Creating incidents
- Recording incident clips
- Uploading clips to Cloudflare R2
- Sending incident metadata to the cloud backend

The engine authenticates with the backend using a **store API key** — generated when the store account is created, stored in the engine's local SQLite database, and sent as an `X-API-Key` header with every outbound request. This is machine-to-machine auth; no token refresh is required.

If the internet is unavailable when an incident is created, the engine retries the upload later using a `synced` flag on each incident record.

#### LENS Interface

A desktop application built with **Qt (C++)** used by store staff.

Responsible for:

- Creating a store account
- Logging in
- Configuring cameras
- Viewing system status
- Managing settings

The Interface and Engine run on the same machine and communicate over **localhost only**. No internet communication is required between the Interface and Engine.

```text
Interface → localhost API → Engine
```

#### Local SQLite Database (Engine)

The engine stores all local state in a SQLite database:

```sql
cameras  (id, name, location, streamUrl, username, password, status)
incidents (id, cameraId, timestamp, clipPath, incidentType, synced)
config   (key, value)   -- store API key and other settings
```

---

### 2. Cloud Backend

Hosted on **Railway**.

Responsible for:

- Authentication
- User accounts
- Store accounts
- Incident metadata
- Issuing presigned URLs for video access
- Push notifications (via FCM)
- Authorization

The backend exposes HTTPS APIs only. The Engine sends outbound HTTPS requests to the backend. The Mobile App communicates only with the backend. **The backend never directly communicates with the Store Server.**

---

### 3. Cloud SQL Database

Hosted in the cloud.

Stores:

```sql
users     (id, email, passwordHash, storeId)
stores    (id, storeName, address, phoneNumber, apiKey)
incidents (id, storeId, cameraId, timestamp, clipPath, incidentType)
```

Camera configuration is stored locally on the engine only and is not synced to the cloud.

---

### 4. Cloudflare R2 (Video Storage)

Incident video clips are stored in **Cloudflare R2**.

- The engine uploads clips directly to R2 after an incident is created.
- The SQL database stores only the clip path (e.g. `incidents/store_45/inc_123.mp4`).
- The mobile app never receives a direct R2 URL. Instead, the backend issues a **presigned URL** with a short TTL (5 minutes). The mobile app streams the clip directly from R2 using that URL.
- R2 has no egress fees, which makes it well-suited for video streaming.

```text
Backend endpoint:   GET /incidents/:id/clip
                    Authorization: Bearer <user JWT>

Response:           { url: "https://r2.example.com/...?expires=300" }

Mobile App streams directly from R2 — video never passes through the backend.
```

---

### 5. Mobile App

Built with **React Native**, published on the Google Play Store.

Users sign in using the same account created through the desktop Interface.

The app communicates only with the Cloud Backend — never directly with the Store Server.

Responsible for:

- User login
- Receiving push notifications (via FCM)
- Viewing the incident list
- Streaming incident video clips via presigned R2 URLs
- Updating incident status

---

## Key Flows

### Account Creation

```text
1. Staff opens LENS Interface
2. Enters store details, email, and password
3. Interface sends registration request to Backend
4. Backend creates user + store records in SQL
5. Backend generates a store API key and returns it
6. Interface saves the API key to the Engine via localhost
7. Engine writes the API key to its local config table
```

### Camera Configuration

```text
1. Staff opens LENS Interface
2. Adds camera details (name, location, stream URL, credentials)
3. Interface sends configuration to Engine via localhost
4. Engine saves camera to local SQLite database
5. Engine connects to the camera stream
```

### Incident Detection

```text
1. Cameras stream video to Engine
2. Engine analyzes frames with AI model
3. Suspicious activity detected
4. Engine records a short video clip locally
5. Engine uploads clip to Cloudflare R2
6. Engine sends incident metadata to Backend (X-API-Key header)
7. Backend stores clip path in SQL
8. Backend sends push notification via FCM to user's mobile device
```

### Incident Viewing

```text
1. User opens Mobile App
2. App requests incident list from Backend (JWT auth)
3. Backend returns incident metadata
4. User selects an incident
5. App requests clip access from Backend (JWT auth)
6. Backend verifies the user belongs to the incident's store
7. Backend generates a presigned R2 URL (5 min TTL)
8. App streams clip directly from R2
```

---

## AI Model

The current proof of concept uses a pretrained video model fine-tuned for suspicious concealment detection.

```text
Video Clip
    ↓
Frame Sampling
    ↓
VideoMAE Model
    ↓
NORMAL / SUSPICIOUS
```

---

## Technology Stack

### Engine

- C++
- OpenCV
- VideoMAE / ONNX Runtime
- SQLite (local camera config, incidents, API key)
- HTTPS (outbound to backend)
- Multithreading

### Desktop Interface

- Qt (C++)
- Communicates with Engine via localhost REST API
- Communicates with Backend via HTTPS for account management

### Backend

- Node.js / Express
- PostgreSQL
- JWT authentication (mobile users)
- API key authentication (engine)
- FCM push notifications
- Cloudflare R2 presigned URL generation

### Mobile App

- React Native
- JWT authentication
- FCM push notifications
- Video streaming via presigned R2 URLs

### Cloud Infrastructure

- **Backend hosting:** Railway
- **Database:** PostgreSQL (cloud-hosted)
- **Video storage:** Cloudflare R2
- **Push notifications:** Firebase Cloud Messaging (FCM)

---

## Project Structure

```text
LENS/
│
├── app/
├── backend/
├── docs/
│   ├── LENS.drawio
│   ├── LENS.drawio.png
│   ├── Logo.jpg
│   └── README.md
├── engine/
│   ├── CMakeLists.txt
│   ├── includes/
│   │   └── camera/
│   │       ├── CameraCapture.h
│   │       ├── CameraConfig.h
│   │       ├── CameraManager.h
│   │       ├── CameraStore.h
│   │       └── CameraWorker.h
│   ├── run.sh
│   └── src/
│       ├── camera/
│       │   ├── CameraCapture.cpp
│       │   ├── CameraManager.cpp
│       │   ├── CameraStore.cpp
│       │   └── CameraWorker.cpp
│       └── main.cpp
├── interface/
└── training/
```

---

## MVP Goal

The MVP focuses on the core event-detection workflow:

- Configure store cameras through the Qt desktop Interface
- Engine connects to cameras and runs AI inference
- Detect suspicious concealment events
- Save incident clips to Cloudflare R2
- Store incident metadata in the cloud database
- Send push notifications to store staff via FCM
- Allow staff to review clips in the React Native mobile app
