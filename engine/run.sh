#!/bin/bash

set -e

ORT_ROOT="/home/braedonk/CET-CS/Level05/Project1/POC/onnxruntime-linux-x64-1.26.0"

echo "Configuring project with CMake..."
cmake -S . -B build \
  -DENABLE_INFERENCE=ON \
  -DORT_ROOT="$ORT_ROOT"

echo "Building project..."
cmake --build build -j$(nproc)

echo "Running LENS..."
LD_LIBRARY_PATH="$ORT_ROOT/lib" ./build/LENS
