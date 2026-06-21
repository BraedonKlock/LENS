#!/bin/bash
set -e
echo "Installing dependencies..."
npm install
echo "Starting LENS mobile..."
npx expo start --dev-client
