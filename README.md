# TLM Window Control (Prototype)

Minimal Qt6 + QML project skeleton for a Windows Window-control app.

## Requirements
- Qt 6 (e.g., MSVC build)
- CMake 3.16+
- Visual Studio / MSVC toolchain on Windows

## Build (Windows)
```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

App entry: `tlmapp` executable (Qt Quick QML UI).

## Notes
This skeleton includes a minimal bridge between QML and a C++ `WindowManager` that can set window transparency on Windows (using Win32 APIs). Continue by implementing window listing, screenshot capture and preset management.
