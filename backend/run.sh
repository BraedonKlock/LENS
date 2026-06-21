#!/bin/bash
set -e
echo "Installing dependencies..."
npm install
echo "Starting LENS backend..."
node app.js
