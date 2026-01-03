#!/bin/bash
export PATH=/mingw64/bin:$PATH
cd "/c/Users/Hayyan/Desktop/C++ Game/sfml-pong"
mkdir -p build_fix
cd build_fix
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --target cube -j 4 > build_log.txt 2>&1
