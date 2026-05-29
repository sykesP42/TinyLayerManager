# TLM Window Control — UI Redesign Spec

## Overview

Redesign the TLM app with a modern, lightweight UI featuring custom window controls, a theme system, and a preview toggle. Goal: small, elegant, programmatic theme switching with zero UI duplication.

## Architecture

### Theme System

**ThemeManager** (C++ QObject singleton, registered as QML context property) exposes all visual attributes as Q_PROPERTY:

```
ThemeManager
  ├── theme: "dark" | "light" | "glass"     (persisted to QSettings)
  ├── previewEnabled: bool                    (persisted to QSettings)
  ├── Colors: bgColor, panelColor, cardColor, textColor,
  │          textSecondaryColor, accentColor, accentTextColor,
  │          borderColor, dangerColor, successColor
  ├── Shape: radius (border radius in px)
  └── Slider: sliderTrackColor, sliderHandleColor
```

- All QML components bind to `ThemeManager.xxx` — no hardcoded colors
- Switching theme = `ThemeManager.theme = "light"` → all bindings re-evaluate → global update
- Themes defined as C++ method `applyTheme("dark")` that sets all properties at once
- Zero QML duplication: one set of components, N theme value sets

### Theme Value Table

| Property        | Dark         | Light        | Glass              |
|-----------------|-------------|-------------|-------------------|
| bgColor         | #1e1e2e     | #f5f5f5     | rgba(20,20,40,0.85) |
| panelColor      | #181825     | #ffffff     | rgba(255,255,255,0.08) |
| cardColor       | #313244     | #f0f0f0     | rgba(255,255,255,0.12) |
| textColor       | #cdd6f4     | #333333     | #ffffff |
| textSecondary   | #6c7086     | #888888     | rgba(255,255,255,0.6) |
| accentColor     | #89b4fa     | #4f8cff     | #89b4fa |
| borderColor     | #45475a     | #e0e0e0     | rgba(255,255,255,0.15) |
| dangerColor     | #f38ba8     | #e53935     | #f38ba8 |
| successColor    | #a6e3a1     | #43a047     | #a6e3a1 |
| radius          | 6px         | 10px        | 10px |

Glass theme additionally applies: `background: blur(20px)` on panels, `opacity: 0.92` on window.

### Window Frame

- `flags: Qt.FramelessWindowHint | Qt.Window` — remove system title bar
- Custom title bar (38px) with drag support via `Window.onActiveChanged` + mouse move
- Title bar elements:
  - Left: app icon + "TLM" label
  - Right: theme toggle button → dropdown with 3 options, separator, minimize, maximize, close
- Window resize: 8 invisible `MouseArea` handles on edges/corners

### Layout

```
+--------------------------------------------------+
|  [icon] TLM          [theme] | [-] [□] [×]       |  ← custom title bar (38px)
+--------------------------------------------------+
|  Search windows...   |  [icon] Window Title      |
|  ─────────────────  |  Code.exe · PID 12345      |
|  ● Visual Studio Code [Cap] |                    |
|  ● Chrome             [Cap] |  Opacity  █████ 192|
|  ● Notepad++          [Cap] |  [Apply][Capture]   |
|  ● Terminal           [Cap] |  [Front][Save]      |
|  ─────────────────  |  ┌─Preview──────[ON|OFF]─┐ |
|  4 windows           |  │                       │ |
|                      |  │   window screenshot    │ |
|                      |  │                       │ |
|                      |  └───────────────────────┘ |
+--------------------------------------------------+
```

### Preview Toggle

- Toggle switch in preview area header (right side)
- **ON**: selecting a window auto-captures screenshot, shows in preview area
- **OFF**: no auto-capture, preview area shows "Preview disabled" hint, Capture button still works manually
- Persisted via `ThemeManager.previewEnabled` → QSettings

### QML Files

| File | Purpose |
|------|---------|
| `main.qml` | ApplicationWindow (frameless), title bar, split layout |
| `ThemeManager.qml` or C++ ThemeManager | Singleton with all theme properties |

Single QML file approach preferred — keep it simple. ThemeManager as C++ class.

## Implementation Steps

1. Create C++ `ThemeManager` class (Q_PROPERTY for all colors, theme switching, QSettings persistence, preview toggle)
2. Rewrite `main.qml`: frameless window, custom title bar with drag/resize, theme dropdown, modern split layout
3. Update `main.cpp`: register ThemeManager, set `Qt::FramelessWindowHint`
4. Update `CMakeLists.txt` if new files added
5. Remove old `resources.qrc` references to `test_minimal.qml`
6. Build and test all 3 themes

## Constraints

- Qt6 / QML only — no external CSS, images, or icon fonts
- All visuals via QML rectangles, shapes, unicode characters
- ThemeManager must be C++ (not QML singleton) for QSettings persistence
- Keep exe lightweight — no QQC2 extra imports, minimal components
