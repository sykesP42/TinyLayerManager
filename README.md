# TLM — Tiny Layer Manager

A lightweight Windows tool for managing window transparency, color tint overlays, and window state presets. Built with ImGui + D3D11.

## Features

- **Window Transparency** — adjust per-window opacity with real-time preview
- **Color Tint Overlay** — apply RGB color tint to any window via overlay layer
- **Window Control** — always-on-top, minimize, maximize, restore, close
- **Preset System** — save and restore complete window state snapshots
- **Per-Window Settings** — persistent settings per executable/window title
- **Theme System** — Dark, Light, and Glass UI themes
- **Live Refresh** — auto-update overlays on window movement/resize

## Requirements

- Windows 10+
- CMake 3.16+
- MinGW or MSVC toolchain

## Build (MinGW)

```bash
mkdir build_mingw && cd build_mingw
cmake -G "MinGW Makefiles" ..
cmake --build . --config Release
```

## Build (MSVC)

```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

## License

See [LICENSE](LICENSE).
