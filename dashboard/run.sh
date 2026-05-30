#!/bin/bash

set -e

echo "Removing old build folder..."
rm -rf build

echo "Configuring project with CMake and Ninja..."
cmake -S . -B build -G Ninja

echo "Building project..."
cmake --build build

echo "Running LENS..."
./build/LENS
