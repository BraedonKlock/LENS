# LENS

**Loss-prevention Event Notification System**

LENS is an AI-powered retail loss-prevention platform designed to help businesses detect suspicious behaviour in real time using existing security camera infrastructure.

Unlike traditional security systems that simply record footage for later review, LENS continuously analyzes live camera feeds, identifies suspicious behaviour patterns, records evidence, and generates alerts for staff review.

LENS is designed as a local, plug-and-play appliance that runs entirely on-site without requiring cloud infrastructure.

## System Architecture

LENS consists of two separate applications running on the same local server.

### LENS Engine

The **LENS Engine** is the core backend service.

It handles:

- Camera management
- Video decoding
- OpenCV processing
- AI inference with YOLO
- Behaviour analysis
- Incident creation
- Video recording
- Database management
- Alert generation
- Shared memory management

### LENS Dashboard

The **LENS Dashboard** is a Qt desktop application.

It handles:

- Live camera viewing
- Incident review
- Camera configuration
- System monitoring
- Settings management
- Alert review

## Communication Architecture

### Commands and Configuration

Dashboard
→ localhost API
→ LENS Engine

### Alerts and Events

LENS Engine
→ WebSocket
→ Dashboard

### Live Video Preview

LENS Engine
→ Shared Memory
→ Dashboard

## Planned Technology Stack

### Core Engine

- C++
- OpenCV
- YOLO
- PostgreSQL
- REST API (localhost)
- WebSockets
- Shared Memory
- Multithreading

### Dashboard

- Qt

## Project Directory Structure

```text
LENS/
│
├── dashboard
│   ├── CMakeLists.txt
│   ├── includes
│   │   └── MainWindow.h
│   ├── run.sh
│   ├── src
│   │   ├── main.cpp
│   │   └── MainWindow.cpp
│   └── ui
│       └── MainWindow.ui
├── docs
│   └── README.md
└── engine
    ├── CMakeLists.txt
    ├── includes
    │   └── camera
    │       ├── CameraCapture.h
    │       ├── CameraConfig.h
    │       ├── CameraManager.h
    │       ├── CameraStore.h
    │       └── CameraWorker.h
    ├── run.sh
    └── src
        ├── camera
        │   ├── CameraCapture.cpp
        │   ├── CameraManager.cpp
        │   ├── CameraStore.cpp
        │   └── CameraWorker.cpp
        └── main.cpp
```


