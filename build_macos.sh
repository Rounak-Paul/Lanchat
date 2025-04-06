#!/bin/bash
echo "Compiling ChatApp for macOS..."

clang++ ChatApp.cpp -o ChatApp -std=c++17 -pthread

if [ $? -ne 0 ]; then
    echo "Compilation failed."
else
    echo "Build successful! Output: ./ChatApp"
fi
