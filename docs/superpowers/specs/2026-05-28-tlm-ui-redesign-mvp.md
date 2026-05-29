# TLM UI/UX Redesign — MVP Specification

## Overview

Rewrite the ImGui-based UI layer of TLM (Window control tool) to fix layout, font, and interaction issues. Add opacity presets, per-window RGB color tint, and a preset system that saves complete window-state snapshots.

## Architecture

### File changes

| File | Action |
|------|--------|
| `src/UI.cpp` | Complete rewrite — new layout, collapsible sections, icon rendering |
| `src/UI.h` | Minor updates for new function signatures |
| `src/Theme.cpp` | Unchanged |
| `src/PresetManager.h/.cpp` | Extend preset format to include all settings (alpha, top, tint) |
| `src/WindowOperator.h/.cpp` | Add `setWindowTint` method for color overlay window |
| `src/main.cpp` | Add D3D11 texture support for window icons |

## Right Panel Layout

Collapsible sections using `ImGui::CollapsingHeader`:

```
[Window Icon (32x32)] [Title]          ← always visible
[EXE path | Position | PID]            ← always visible

[Front | Min | Max | Restore] [✕]      ← always visible

[✓ Always on Top] [Reset All]          ← always visible, compact

▼ Opacity                              ← DEFAULT EXPANDED
  [100%] [80%] [50%] [25%]            ← preset buttons
  [=========slider=========] 50%      ← slider + percentage

▸ Color Tint                           ← collapsed by default
  [Normal] [Warm] [Cool] [Night] [+]
  R [══════════════] 255
  G [═══════░░░░░░░] 200
  B [══════════════] 255
  Intensity [════░░░] 40%

▸ Saved Presets                        ← collapsed by default
  [Save current as..._________] [+]
  reading                           x
  sunset                            x

[↻ Refresh Window List]                ← always visible at bottom
```

### Per-window icon

Extract HICON via `ExtractIconExW` from the exe path. Convert to D3D11 texture (`ID3D11ShaderResourceView`) for display via `ImGui::Image`. Fallback to a colored square with first letter if icon extraction fails.

## Data Model

### Per-window settings (persisted)

File: `%APPDATA%/TLM/perwindow.txt`
Format: `exe\x0title|alpha|top|tintR|tintG|tintB|tintIntensity`

```
notepad.exe\x0无标题 - 记事本|255|0|255|255|255|0
```

Loaded on window selection, saved on slider/button change. Keyed by `exe + '\x0' + title`.

### Presets (persisted)

File: `%APPDATA%/TLM/presets.txt` (already exists, extend format)

New format: `name|targetTitle|alpha|top|tintR|tintG|tintB|tintIntensity`

```
reading|notepad.exe|180|0|255|200|150|40
```

A preset saves: **opacity** + **always-on-top** + **tint R/G/B** + **tint intensity**.
Applying a preset sets all these values on the current window.

**Note**: Old presets from the previous format (`name|title|alpha`) will fail to parse. Since this is an MVP breaking change, old presets are discarded gracefully (empty preset list shown). The file is overwritten on the next save with the new format.

### Color tint presets (built-in)

| Name | R | G | B | Intensity |
|------|---|---|---|-----------|
| Normal | 255 | 255 | 255 | 0 |
| Warm | 255 | 220 | 180 | 30 |
| Cool | 200 | 220 | 255 | 30 |
| Night | 180 | 200 | 255 | 50 |

## Color Tint Implementation

Windows does not provide a native API for per-channel RGB adjustment. Approach:

1. Create one `WS_EX_LAYERED` popup per tinted window, positioned as an overlay
2. **Overlay properties**: `WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST` — passes all mouse events through to the target window
3. **Position tracking**: timer every 500ms calls `GetWindowRect` on target HWND and repositions overlay with `SetWindowPos`
4. **Z-order**: overlay stays on top of target but below other topmost windows (`HWND_TOPMOST` + `SWP_NOACTIVATE`)
5. Fill overlay with the tint color at the specified alpha via `UpdateLayeredWindow`
6. When target window is destroyed, timer detects invalid HWND and destroys the overlay

### Color model

```
overlayColor = (255 - R, 255 - G, 255 - B)
overlayAlpha = intensity
```

- R/G/B = 255, Intensity = 0 → overlay fully transparent (no effect)
- R = 128, G = 255, B = 255, Intensity = 30 → cyan overlay, reduces perceived red
- R = 255, G = 128, B = 255, Intensity = 40 → magenta overlay, reduces perceived green

### Multiple tinted windows

Each tinted target window gets its own overlay HWND. Managed via a map of `HWND_target → {overlay_hwnd, tintR, tintG, tintB, intensity}`.

## Handling Edge Cases

- **No window selected**: Right panel shows "Select a window from the list"
- **Empty window list**: Left panel shows "No windows found"
- **Preset with no name**: Save button disabled when input is empty
- **Overlay window when target is destroyed**: Timer detects invalid HWND, destroys overlay
- **Toggle Color Tint off**: Setting all R/G/B to 255 / Intensity 0 destroys overlay window
- **Per-window settings miss**: Default to opacity=255, top=off, tint=Normal
- **Corrupt preset file**: Silently reset, file regenerated on next save

## UI States

### Empty state (no windows detected)
```
Left:  "No windows found"
Right: "Select a window from the list"
```

### Loading state (enumeration in progress)
```
Left:  "No windows found" (brief flash, < 100ms typically)
       → list populates automatically when enumeration completes
```

### Active state (window selected, all controls functional)
See layout above.

### Multi-select state
Batch action bar appears at bottom of left panel:
`[Front] [Min] [X] [50%] [80%] [100%] [T]`

## Left Panel Layout

```
[Search...                     ]    ← always visible, filters in real-time
┌──────────────────────────────┐
│  ☐ [icon] Code.exe - main   │    ← 30px per item, hover highlight
│  ☑ [icon] Terminal          │    ← selected = accent bg, multi = green bg
│  ☐ [icon] chrome.exe        │
└──────────────────────────────┘
5 windows (1 selected) [All][None]
[Front] [Min] [X] [50%] [80%] [100%] [T]   ← batch bar (only when multi-select)
```

## Interaction Rules

- **CollapsingHeader click**: Toggle section open/closed
- **Preset chip click**: Apply the preset to current window immediately
- **Opacity slider drag**: Real-time window transparency update
- **Tint slider drag**: Real-time color overlay update
- **R/G/B at 255 + Intensity 0**: Remove overlay (Normal preset)
- **Close confirm dialog**: Appears on X button click, requires double-confirmation
- **Splitter drag**: Adjust left panel width (clamped 220–400px)

## Font

Load Microsoft YaHei (msyh.ttc) at 14px with `GetGlyphRangesChineseSimplifiedCommon()`. Fallback chain: msyh.ttc → simsun.ttc → deng.ttf → default font.

## Window List Refresh

- Initial: immediate on startup
- Timer: every 2 seconds
- Triggered by WinEventMonitor (window create/destroy/rename)
- Always runs on background thread, posts results to main thread via `WM_APP + 1`
