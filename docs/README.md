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
