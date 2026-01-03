# SFML Pong (minimal)

A tiny Pong clone using **SFML** + **CMake**, ready for development in **VS Code**.

## What I added ✅
- `CMakeLists.txt` — CMake build file
- `src/main.cpp` — minimal Pong game (W/S and Up/Down to move paddles)
- `assets/` — place a `.ttf` font here (e.g., `assets/arial.ttf`) to display scores
- `.vscode/tasks.json` & `.vscode/launch.json` — basic helper tasks for build & debug

---

## Build (Windows) — recommended via vcpkg

1. Install vcpkg: https://github.com/microsoft/vcpkg
2. Install SFML (x64) with vcpkg:

   vcpkg install sfml[graphics,window,audio]:x64-windows

3. Configure CMake (replace path to your vcpkg root):

   cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

4. Build:

   cmake --build build --config Release

5. Run the executable at `build/Release/pong.exe` (or run from VS Code debugger).

---

## Notes
- Put a TTF file (e.g., `arial.ttf`) into `assets/` for the score text, or the game will run but won't display the score (a warning is printed).
- Recommended VS Code extensions: **C/C++**, **CMake Tools**.

---

Have it running? Tell me if you want: a pause menu, AI paddle, nicer collision, or sound. 🎮
