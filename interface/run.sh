#!/bin/bash
set -e
echo "Building LENS Interface..."
cmake -B build
cmake --build build
echo "Launching LENS Interface..."
./build/LENSInterface
