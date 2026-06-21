#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

curl -s -X POST http://127.0.0.1:7373/test/video \
  -H "Content-Type: application/json" \
  -d "{\"path\":\"$SCRIPT_DIR/models/test.mp4\",\"camera_id\":\"1\"}" | python3 -m json.tool
