#!/bin/bash
# Cross-compile textrayted for Windows using mingw-w64

set -e

echo "Downloading raylib Windows libraries..."
mkdir -p winlibs
cd winlibs
wget -q https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip
unzip -q raylib-5.5_win64_mingw-w64.zip
cd ..

echo "Compiling..."
x86_64-w64-mingw32-gcc -Wall -O2 -std=c11 -mwindows \
  -Iwinlibs/raylib-5.5_win64_mingw-w64/include \
  -Lwinlibs/raylib-5.5_win64_mingw-w64/lib \
  -static -static-libgcc \
  -o textrayted.exe main.c \
  -lraylib -lwinmm -lgdi32 -lshell32 -lopengl32

echo "Creating distribution zip..."
zip -r textrayted-windows.zip textrayted.exe README-windows.txt 2>/dev/null || true

echo "Done: textrayted.exe (standalone) and textrayted-windows.zip"