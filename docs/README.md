# LENS

**Loss-prevention Event Notification System**

LENS is an AI-powered retail loss-prevention system that uses existing security camera feeds to detect suspicious concealment events and notify users with a short video clip for review.

Instead of replacing a store’s current DVR/NVR system, LENS runs alongside it. The existing security system continues handling live viewing and recording, while LENS focuses on event detection, incident clips, and alerts.

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
User Reviews Clip in React Native App
```

---

## Architecture

LENS is made up of four main parts:

### LENS Engine

The local engine runs on-site and handles:

- Camera stream connection
- Video processing
- AI inference
- Suspicious event detection
- Incident clip creation
- Local storage
- Sending incidents to the backend

### LENS Desktop App

The desktop app is built with **Electron.js** and **React**. It acts as the local setup and configuration tool for store camera connections.

The desktop app handles:

- Adding store cameras
- Editing camera connection details
- Removing cameras
- Testing camera connections
- Configuring camera names and locations
- Sending camera setup information to the local C++ LENS Engine

The desktop app does not perform AI detection itself. The C++ LENS Engine owns the camera connections, video processing, AI inference, recording, and incident creation.

### LENS Backend

The backend handles:

- User accounts
- Store accounts
- Subscriptions
- Incident metadata
- Notifications
- App authentication

### LENS Mobile App

The mobile app is built with **React Native** and handles:

- User login
- Incident notifications
- Incident list
- Video clip review
- Incident status updates

---

## AI Model

The current proof of concept uses a pretrained video model fine-tuned for suspicious concealment detection.

Current model pipeline:

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

## Planned Technology Stack

### Engine

- C++
- OpenCV
- VideoMAE
- ONNX Runtime
- SQLite or PostgreSQL
- REST API
- Multithreading

### Training

- Python
- PyTorch
- HuggingFace Transformers
- ONNX export

### Desktop App

- Electron.js
- React
- HTML / CSS / JavaScript
- Local REST API communication with the C++ engine

### Backend

- Node.js / Express
- PostgreSQL
- Authentication
- Subscription integration
- Push notifications

### Mobile App

- React Native
- Video playback
- Push notifications
- Incident review interface

---

## Project Structure

```text
LENS/
│
├── app
├── backend
│   ├── api
│   │   └── loggedIn.js
│   ├── app.js
│   └── controllers
│       └── loggenIn.js
├── docs
│   └── README.md
├── engine
│   ├── CMakeLists.txt
│   ├── includes
│   │   └── camera
│   │       ├── CameraCapture.h
│   │       ├── CameraConfig.h
│   │       ├── CameraManager.h
│   │       ├── CameraStore.h
│   │       └── CameraWorker.h
│   ├── run.sh
│   └── src
│       ├── camera
│       │   ├── CameraCapture.cpp
│       │   ├── CameraManager.cpp
│       │   ├── CameraStore.cpp
│       │   └── CameraWorker.cpp
│       └── main.cpp
├── interface
└── training
```

---

## MVP Goal

The MVP focuses on the core event-detection workflow:

- Configure store cameras through the Electron desktop app
- Send camera setup information to the local C++ engine
- Connect to a camera feed
- Run AI inference on video clips
- Detect suspicious concealment events
- Save incident clips
- Send notifications
- Allow users to review clips in the React Native app
