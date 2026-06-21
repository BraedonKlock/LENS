# LENS Architecture

This document explains the full system architecture of LENS — every component, how they communicate, how authentication works, how video is stored and delivered, and the reasoning behind each decision.

---

## System Overview

LENS is split across two physical locations: the store and the cloud.

```text
┌──────────────────────────────────┐        ┌───────────────────────────────────┐
│        STORE (On-Premise)        │        │               CLOUD               │
│                                  │        │                                   │
│  ┌─────────────┐   localhost      │        │  ┌────────────┐                  │
│  │  Interface  │◀──────────────▶ Engine   │  │  Backend   │                  │
│  └─────────────┘   :7373         │  HTTPS │  │ (Railway)  │                  │
│                                  │───────▶│  └─────┬──────┘                  │
│  ┌─────────────┐                 │        │        │                          │
│  │   Engine    │─────────────────┼───────▶│  ┌─────▼──────┐  ┌────────────┐  │
│  └──────┬──────┘  HTTPS          │        │  │   MySQL    │  │Cloudflare  │  │
│         │                        │        │  └────────────┘  │    R2      │  │
│  ┌──────▼──────┐                 │        │                  └─────▲──────┘  │
│  │   SQLite    │                 │        │                        │          │
│  └─────────────┘                 │        │  ┌────────────┐  presigned URL   │
│                                  │        │  │ Mobile App │────────┘          │
│  ┌─────────────┐                 │        │  └─────┬──────┘                  │
│  │  Cameras    │──▶ Engine       │        │        │ HTTPS                   │
│  └─────────────┘   (RTSP)        │        │  ┌─────▼──────┐                  │
│                                  │        │  │  Backend   │                  │
└──────────────────────────────────┘        │  └────────────┘                  │
                                            └───────────────────────────────────┘
```

The backend never reaches into the store. All communication is outbound from the store to the cloud.

---

## Components

### 1. LENS Engine

The engine is the core of the system. It runs as a background process on the store server and handles everything related to cameras and AI detection.

**Responsibilities:**
- Connecting to RTSP/HTTP camera streams
- Reading frames and maintaining a rolling 5-second buffer per camera
- Running VideoMAE AI inference every 5 frames to detect suspicious concealment
- On detection: encoding a clip, requesting an R2 presigned URL, uploading the clip, reporting to backend
- Exposing a local REST API on `127.0.0.1:7373` for the Interface

**Communication:**
- Listens on `localhost:7373` for commands from the Interface
- Sends outbound HTTPS to the Backend (incident reporting, presigned URL requests)
- Uploads clips directly to Cloudflare R2 via presigned PUT URL

The engine never receives inbound connections from the cloud.

---

### 2. LENS Interface

A Qt (C++) desktop application that runs on the same machine as the Engine.

**Responsibilities:**
- Creating the store account — validates the server ID, creates user + store in the backend, configures the engine with its API key
- Adding and removing cameras
- Viewing live camera connection status (Connected / Connecting...)

**Communication:**
- Engine: `localhost:7373` REST API (camera management, config setup)
- Backend: HTTPS (account registration only)

```text
Interface → localhost:7373 → Engine   (camera config, status)
Interface → HTTPS          → Backend  (register only)
```

---

### 3. Cloud Backend

Node.js / Express API hosted on Railway.

**Responsibilities:**
- Server ID validation (each physical server has a pre-registered unique ID in the DB)
- Store account creation with a unique engine API key
- User login with JWT
- Engine authentication via `X-API-Key` header → `store_id` lookup
- Generating presigned R2 PUT URLs for the engine to upload clips
- Storing incident metadata in MySQL
- Sending Expo push notifications to registered mobile devices
- Serving incident list and presigned R2 GET URLs to the mobile app

**Routes:**

| Prefix | Auth | Used by |
|---|---|---|
| `/auth` | none | Interface (register, login) |
| `/engine` | X-API-Key | Engine (upload request, incident report) |
| `/` | JWT | Mobile app (incidents, push token) |

The backend never initiates contact with the Store Server.

---

### 4. MySQL (Cloud Database)

```sql
servers (
    id          VARCHAR(50) PRIMARY KEY,  -- pre-registered e.g. "LENS-STORE-0001"
    in_use      TINYINT(1) DEFAULT 0,
    store_id    VARCHAR(36) REFERENCES stores(id),
    created_at  TIMESTAMP
)

stores (
    id           VARCHAR(36) PRIMARY KEY,
    store_name   VARCHAR(255) NOT NULL,
    store_number VARCHAR(50) NOT NULL,
    api_key      VARCHAR(255) UNIQUE NOT NULL   -- engine credential
)

users (
    id            VARCHAR(36) PRIMARY KEY,
    store_id      VARCHAR(36) REFERENCES stores(id),
    email         VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL
)

incidents (
    id            VARCHAR(36) PRIMARY KEY,
    store_id      VARCHAR(36) REFERENCES stores(id),
    camera_id     VARCHAR(100),
    timestamp     DATETIME NOT NULL,
    clip_path     TEXT NOT NULL,              -- e.g. clips/{store_id}/{timestamp}_cam1.mp4
    incident_type VARCHAR(100),
    reviewed      TINYINT(1) DEFAULT 0
)

push_tokens (
    id        VARCHAR(36) PRIMARY KEY,
    user_id   VARCHAR(36) REFERENCES users(id),
    token     VARCHAR(255) UNIQUE NOT NULL    -- Expo push token
)
```

Camera configuration is **not** stored in the cloud. It lives only on the engine's local SQLite.

---

### 5. Engine SQLite Database (Local)

```sql
cameras (
    id        INTEGER PRIMARY KEY,
    name      TEXT NOT NULL,
    url       TEXT NOT NULL,
    password  TEXT,
    location  TEXT
)

config (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
    -- keys: "api_key", "backend_url"
)
```

Camera credentials never leave the store server.

---

### 6. Cloudflare R2 (Video Storage)

Incident clips are stored in R2 under a per-store folder.

**Clip path format:**
```text
clips/{store_id}/{timestamp}_cam{cameraId}.mp4
```

**Why R2:**
- No egress fees — video streaming costs nothing regardless of volume
- S3-compatible API — standard AWS SDK works out of the box
- Presigned URL support with configurable TTL

The mobile app never receives a permanent R2 URL. The backend always generates a short-lived presigned GET URL (5-minute TTL) on demand. Video never passes through the backend.

---

### 7. Mobile App

React Native (Expo SDK 54).

**Responsibilities:**
- User login (JWT stored in SecureStore)
- Push token registration with backend on login
- Receiving Expo push notifications when incidents are detected
- Fetching incident list from backend (re-fetches on foreground resume)
- Streaming incident clips via presigned R2 URLs
- Marking incidents as reviewed

**Communication:**
- All requests go to the Backend only — the app never talks to the Store Server or R2 directly
- Video is streamed from R2 using presigned URLs issued by the backend

---

## Authentication

### Mobile App → Backend: JWT

User logs in with email + password. Backend returns a signed JWT (7-day expiry). App includes it in every request:

```text
Authorization: Bearer <jwt_token>
```

JWT payload includes `userId` and `storeId` so the backend can scope all data to the correct store.

### Engine → Backend: Store API Key

The engine is an always-on background process — not a user session. It uses a static store API key:

```text
X-API-Key: <64-char hex key>
```

The key is:
1. Generated by the backend at account creation (`crypto.randomBytes(32).hex()`)
2. Returned to the Interface via the register response
3. Sent by the Interface to the engine via `POST localhost:7373/config/setup`
4. Persisted in the engine's local SQLite `config` table
5. Loaded on engine startup and passed to every CameraWorker and IncidentHandler

The `engineAuth` middleware does a single DB lookup: `SELECT store_id FROM stores WHERE api_key = ?`. Every subsequent operation is scoped to that `store_id`.

**Why not JWT for the engine:** JWT is designed for user sessions with expiry and refresh. A background machine process running 24/7 without user interaction doesn't fit that model. A static API key is the standard pattern for machine-to-machine auth.

### Server ID Claim (Account Creation)

Each physical server ships with a unique ID pre-registered in the `servers` table (`in_use = 0`). When staff create an account they enter this ID. The backend:

1. Verifies the server ID exists and `in_use = 0`
2. Creates the store and user in a transaction
3. Sets `in_use = 1` and links `store_id`

This prevents any second user from claiming the same server and ensures one store account per physical server.

---

## AI Inference

The engine runs a fine-tuned **VideoMAE** binary classification model exported to ONNX.

```text
Input:   16 frames sampled evenly from the 5-second camera buffer
         Resized to 224×224, BGR→RGB, ImageNet normalised
         Tensor shape: [1, 16, 3, 224, 224]

Output:  logits [normal, suspicious]
         Softmax → P(suspicious)
         Threshold: 0.5
```

Inference runs every 5 frames per camera on a dedicated worker thread. A 30-second cooldown prevents the same event triggering multiple incidents.

Model: `model.onnx` + `model.onnx.data` (334 MB weights), loaded via ONNX Runtime 1.26.

---

## Video Delivery

```text
1. Mobile app:   GET /incidents/:id/clip
                 Authorization: Bearer <jwt>

2. Backend:      Verifies JWT
                 Confirms incident.store_id == user.store_id
                 Generates presigned R2 GET URL (5-min TTL)

3. Returns:      { "url": "https://r2.cloudflare.com/clips/...?X-Amz-Expires=300&..." }

4. Mobile app streams video directly from R2
```

The backend never touches the video bytes — no memory or bandwidth overhead on the server.

---

## Key Flows

### Account Creation

```text
1. Staff opens Interface, enters: Server ID, email, password, store name, store number
2. Interface → POST /auth/register → Backend
3. Backend validates Server ID is registered and not in_use
4. Backend transaction:
   - INSERT INTO stores (generates api_key)
   - INSERT INTO users
   - UPDATE servers SET in_use=1, store_id=<new store>
5. Backend returns jwt + engine_api_key
6. Interface → POST localhost:7373/config/setup → Engine
7. Engine saves api_key + backend_url to SQLite config table
```

### Camera Configuration

```text
1. Staff opens Interface, enters camera name, URL, password, location
2. Interface → POST localhost:7373/cameras → Engine
3. Engine saves camera to SQLite cameras table
4. Engine starts a CameraWorker thread immediately
5. Worker connects to the RTSP stream and begins reading frames
```

No internet required. Camera credentials never leave the store server.

### Incident Detection

```text
1. CameraWorker reads frames into a rolling 5-second deque buffer
2. Every 5 frames: Inferencer.classify(buffer)
   - Samples 16 frames evenly, normalises, runs ONNX session
   - Returns P(suspicious)
3. If P ≥ 0.5 AND cooldown has elapsed (30s):
   a. IncidentHandler starts on a detached thread (shared_ptr keeps it alive)
   b. Encodes buffer frames to /tmp/lens_{timestamp}.mp4
   c. POST /engine/incidents/request-upload → Backend returns { uploadUrl, clipPath }
   d. HTTP PUT clip to R2 presigned URL
   e. Deletes local /tmp file
   f. POST /engine/incidents → Backend
      - Backend inserts incident into MySQL
      - Backend queries push_tokens for all users in the store
      - Backend sends Expo push notification to each token
4. Staff phone receives push notification
5. App re-fetches incident list (also re-fetches on foreground resume)
```

### Incident Viewing

```text
1. Staff opens app or taps notification
2. App fetches incident list → GET / incidents (JWT auth)
3. Staff taps incident → GET /incidents/:id/clip (JWT auth)
4. Backend generates presigned R2 GET URL (5-min TTL)
5. App streams clip directly from R2
6. Staff marks as reviewed → PATCH /incidents/:id/review
```

---

## Technology Stack

| Layer | Technology |
|---|---|
| Engine | C++17, OpenCV, ONNX Runtime 1.26, SQLite3, libcurl, cpp-httplib |
| Interface | Qt6 (C++), QNetworkAccessManager |
| Backend | Node.js, Express, MySQL2, JWT (jsonwebtoken), bcrypt, AWS SDK v3 (R2) |
| Mobile App | React Native, Expo SDK 54, expo-notifications, expo-video, expo-secure-store |
| Cloud Database | MySQL |
| Video Storage | Cloudflare R2 |
| Push Notifications | Expo Push Notifications API |
| Backend Hosting | Railway |
| Mobile Builds | EAS Build |

---

## What Is Out of Scope (MVP)

- Subscriptions and billing
- Multi-store management from a single account
- Camera live preview in the mobile app
- Role-based access control (manager vs. staff)
- Audit logs
- Offline incident retry (synced flag — planned but not yet implemented)
- iOS support (Android only for now)
