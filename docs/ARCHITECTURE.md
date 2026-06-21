# LENS Architecture

This document explains the full system architecture of LENS, including every component, how they communicate, how authentication works, how video is stored and delivered, and the reasoning behind each decision.

---

## System Overview

LENS is split across two physical locations: the store and the cloud.

```text
┌─────────────────────────────────┐        ┌──────────────────────────────────┐
│         STORE (On-Premise)      │        │              CLOUD               │
│              ┌──────────────────────────────────────────────────────┐       │ 
│  ┌─────────────┐                │  HTTPS │  ┌────────────┐          |       │
│  │   Engine    │────────────────┼─────────▶ │  Backend   │          |       │
│  └─────────────┘───▶ ┌────────┐ │        │  └─────┬──────┘          |       │
│         │ localhost  │ SQLite │          │        │                 |       │
│  ┌─────────────┐     └────────┘ │        │  ┌─────▼──────┐  ┌──────▼────┐  │
│  │  Interface  │                │        │  │ PostgreSQL │  │Cloudflare  │  │
│  └─────────────┘                │        │  └────────────┘  │    R2      │  │
│                                 │        │                  └─────▲──────┘  │
└─────────────────────────────────┘        │                        │          │
                                           │  ┌────────────┐        │          │
                                           │  │ Mobile App │────────┘          │
                                           │  └─────┬──────┘  (presigned URL)  │
                                           │        │                          │
                                           │  ┌─────▼──────┐                  │
                                           │  │  Backend   │                  │
                                           │  └────────────┘                  │
                                           └──────────────────────────────────┘
```

The Backend never reaches into the store. All communication is outbound from the store to the cloud.

---

## Components

### 1. LENS Engine

The engine is the core of the system. It runs as a background process on the store server and handles everything related to cameras and AI detection.

**Responsibilities:**
- Connecting to camera streams
- Reading and processing video frames
- Running AI inference to detect suspicious activity
- Recording short video clips when events are detected
- Uploading clips to Cloudflare R2
- Sending incident metadata to the backend
- Retrying failed uploads when internet connectivity is restored

**Communication:**
- Listens on localhost for commands from the Interface
- Sends outbound HTTPS requests to the Backend
- Uploads directly to Cloudflare R2

The engine never receives inbound connections from the cloud. It only makes outbound requests.

---

### 2. LENS Interface

The Interface is a Qt (C++) desktop application that runs on the same machine as the Engine. It is used by store staff to set up and manage the system.

**Responsibilities:**
- Creating a store account (sends request to Backend)
- Logging in
- Adding, editing, and removing cameras
- Testing camera connections
- Viewing system status
- Managing settings

**Communication:**
- Talks to the Engine through localhost only — no internet required for camera config
- Talks to the Backend over HTTPS for account creation and login

```text
Interface → localhost REST API → Engine   (camera config)
Interface → HTTPS              → Backend  (account management)
```

---

### 3. Cloud Backend

Hosted on Railway. The backend is the central hub for everything that lives in the cloud.

**Responsibilities:**
- User and store account management
- Authentication (JWT for mobile users, API key for the engine)
- Storing incident metadata
- Generating presigned Cloudflare R2 URLs for video access
- Sending push notifications via Firebase Cloud Messaging (FCM)
- Authorizing access to incidents by store membership

**Communication:**
- Receives inbound HTTPS from the Engine
- Receives inbound HTTPS from the Mobile App
- Writes to PostgreSQL
- Communicates with Cloudflare R2 to generate presigned URLs
- Communicates with FCM to send push notifications

The backend never initiates contact with the Store Server.

---

### 4. PostgreSQL (Cloud Database)

Stores all cloud-side persistent data.

**Schema:**

```sql
users (
    id           UUID PRIMARY KEY,
    email        TEXT UNIQUE NOT NULL,
    passwordHash TEXT NOT NULL,
    storeId      UUID REFERENCES stores(id)
)

stores (
    id          UUID PRIMARY KEY,
    storeName   TEXT NOT NULL,
    address     TEXT,
    phoneNumber TEXT,
    apiKey      TEXT UNIQUE NOT NULL   -- used by the engine to authenticate
)

incidents (
    id           UUID PRIMARY KEY,
    storeId      UUID REFERENCES stores(id),
    cameraId     TEXT,
    timestamp    TIMESTAMPTZ NOT NULL,
    clipPath     TEXT NOT NULL,        -- e.g. incidents/store_45/inc_123.mp4
    incidentType TEXT
)
```

Camera configuration is **not** stored in the cloud database. It lives only on the engine's local SQLite database. This keeps camera credentials off the cloud and simplifies the setup flow.

---

### 5. Engine SQLite Database (Local)

The engine maintains its own local SQLite database on the store server.

**Schema:**

```sql
cameras (
    id        INTEGER PRIMARY KEY,
    name      TEXT NOT NULL,
    location  TEXT,
    streamUrl TEXT NOT NULL,
    username  TEXT,
    password  TEXT,
    status    TEXT DEFAULT 'active'
)

incidents (
    id           INTEGER PRIMARY KEY,
    cameraId     INTEGER REFERENCES cameras(id),
    timestamp    TEXT NOT NULL,
    clipPath     TEXT NOT NULL,    -- local file path before upload
    incidentType TEXT,
    synced       INTEGER DEFAULT 0 -- 0 = not yet uploaded, 1 = uploaded
)

config (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL            -- stores the API key and other settings
)
```

The `synced` column on incidents allows the engine to retry uploads if the internet connection was down when an incident was first detected.

---

### 6. Cloudflare R2 (Video Storage)

Incident video clips are stored in Cloudflare R2, an S3-compatible object store.

**Why R2:**
- S3-compatible API — standard tooling works out of the box
- No egress fees — video streaming does not incur bandwidth costs
- Straightforward presigned URL support

**Clip path format:**
```text
incidents/store_{storeId}/inc_{incidentId}.mp4
```

The PostgreSQL `incidents` table stores this path. The actual video file lives in R2. The mobile app never receives a permanent R2 URL — it always goes through the backend to get a short-lived presigned URL.

---

### 7. Mobile App

Built with React Native, published on the Google Play Store.

**Responsibilities:**
- User login (JWT)
- Receiving push notifications (FCM)
- Viewing the incident list
- Streaming incident video clips via presigned R2 URLs
- Marking incidents as reviewed

**Communication:**
- All requests go to the Backend only
- Video is streamed directly from R2 using presigned URLs issued by the Backend
- The app never communicates with the Store Server

---

## Authentication

LENS uses two separate authentication mechanisms depending on who is making the request.

### Mobile App → Backend: JWT

Standard user authentication. The user logs in with their email and password. The backend returns a signed JWT. The app includes this token in every subsequent request.

```text
Authorization: Bearer <jwt_token>
```

JWTs are short-lived. Refresh tokens can be added later if needed.

### Engine → Backend: Store API Key

The engine is a machine process, not a user. It runs unattended 24/7 and does not have a session. For this reason, the engine uses a static store API key rather than JWT.

The API key is:
- Generated by the backend when the store account is created
- Returned to the Interface, which saves it to the engine via localhost
- Stored in the engine's local `config` SQLite table
- Sent as a header with every request the engine makes to the backend

```text
X-API-Key: <store_api_key>
```

The backend looks up the store by API key and uses it to associate the incoming incident with the correct store.

**Why not JWT for the engine:**
JWT is designed for user sessions — it has an expiry, needs refreshing, and is tied to a login event. An always-on background process does not naturally fit this model. A static API key is simpler, reliable, and the standard pattern for machine-to-machine communication.

---

## Video Delivery

When the mobile app requests a video clip, the backend does not proxy the video. Instead it issues a **presigned URL** directly to Cloudflare R2.

```text
1. App sends:  GET /incidents/:id/clip
               Authorization: Bearer <jwt>

2. Backend:    Verifies JWT
               Checks the incident belongs to the user's store
               Generates a presigned R2 URL with a 5-minute TTL

3. Backend returns:
               { "url": "https://r2.example.com/incidents/store_45/inc_123.mp4?X-Amz-Expires=300&..." }

4. App streams video directly from R2 using that URL
```

**Why presigned URLs:**
- The backend never touches the video bytes — no memory or bandwidth overhead
- The URL expires after 5 minutes so it cannot be shared or reused
- R2's no-egress policy means video streaming is free regardless of volume

---

## Key Flows

### Account Creation

```text
1. Staff opens LENS Interface
2. Enters store name, address, email, and password
3. Interface sends POST /auth/register to Backend
4. Backend creates user and store records in PostgreSQL
5. Backend generates a store API key
6. Backend returns the API key to the Interface
7. Interface sends the API key to the Engine via localhost
8. Engine writes the API key to its local config table
```

### Camera Configuration

```text
1. Staff opens LENS Interface
2. Adds camera details (name, location, stream URL, credentials)
3. Interface sends request to Engine via localhost
4. Engine saves the camera to its local SQLite cameras table
5. Engine connects to the camera stream
```

No internet connection is required for this flow. Camera credentials never leave the store server.

### Incident Detection

```text
1. Cameras stream video to Engine
2. Engine samples frames and runs AI inference
3. Suspicious activity detected
4. Engine records a short video clip to local disk
5. Engine uploads the clip to Cloudflare R2
6. Engine marks the local incident record as synced
7. Engine sends POST /incidents to Backend with:
   - X-API-Key header
   - Incident metadata (cameraId, timestamp, clipPath, incidentType)
8. Backend stores the incident in PostgreSQL
9. Backend sends a push notification via FCM to the store's registered devices
```

If the upload or backend request fails, the `synced` flag stays 0 and the engine retries on its next cycle.

### Incident Viewing

```text
1. User opens Mobile App
2. App sends GET /incidents to Backend (JWT auth)
3. Backend returns incident list for the user's store
4. User taps an incident
5. App sends GET /incidents/:id/clip to Backend (JWT auth)
6. Backend verifies the incident belongs to the user's store
7. Backend generates a presigned R2 URL (5-minute TTL)
8. App streams the clip directly from R2
```

---

## Technology Stack

| Layer             | Technology                          |
|-------------------|-------------------------------------|
| Engine            | C++, OpenCV, ONNX Runtime, SQLite   |
| Desktop Interface | Qt (C++)                            |
| Backend           | Node.js, Express, PostgreSQL        |
| Video Storage     | Cloudflare R2                       |
| Push Notifications| Firebase Cloud Messaging (FCM)      |
| Mobile App        | React Native                        |
| Backend Hosting   | Railway                             |

---

## What Is Intentionally Out of Scope (MVP)

The following are planned for later and are not part of the MVP:

- Subscriptions and billing
- Licensing or seat limits
- Multi-store management from a single account
- Camera feed live preview in the mobile app
- Role-based access control (e.g. manager vs. staff)
- Audit logs
