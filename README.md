<div align="center">

# TLM — Tiny Layer Manager

**轻量级 Windows 窗口管理工具 | Lightweight Windows Window Manager**

[![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)](https://www.microsoft.com/windows)
[![Language](https://img.shields.io/badge/Language-C++17-EF5350?logo=cplusplus)](https://isocpp.org/)
[![Renderer](https://img.shields.io/badge/Renderer-DirectX%2011-4FC3F7?logo=directx)](https://docs.microsoft.com/windows/win32/direct3d11)
[![UI](https://img.shields.io/badge/UI-Dear%20ImGui-FA9302)](https://github.com/ocornut/imgui)
[![Build](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-4DD0E1)](LICENSE)
[![Size](https://img.shields.io/badge/Size-~1.4MB-689F38)]()

基于 Dear ImGui + D3D11 渲染的单文件无依赖桌面工具

</div>

---

## 🇨🇳 中文说明

### 功能特性

#### 透明度控制
- 🎚️ 单窗口不透明度调节（30%–100%）
- ⚡ 快捷预设：100% / 80% / 50% / 25%
- 🔄 一键 Hide/Show 切换所有效果

#### 色彩叠加
- 🎨 为任意窗口叠加 RGB 色彩滤镜
- 🌅 预设模式：正常 / 暖色 / 冷色 / 夜间模式
- 🔧 自定义 R/G/B + 强度调节

#### 窗口定位
- 📍 点击标题栏 `+` 按钮进入定位模式
- 🖱️ 鼠标移动时橙色描边框实时跟随目标窗口
- 📋 左侧列表同步高亮显示
- ✅ 点击锁定选中，ESC / 右键取消

#### 窗口操作
- ⬆️ 置顶 / 最小化 / 最大化 / 还原 / 关闭
- ✂️ 多选批操作：同时对多个窗口执行操作

#### 预设 & 持久化
- 💾 保存/恢复完整窗口状态快照（透明度 + 色调 + 置顶）
- 🧠 按 exe + 标题自动记忆每个窗口的设置

#### 界面定制
- 🎭 三套主题：深色 / 浅色 / 玻璃
- 🔍 搜索过滤：按窗口标题或进程名实时筛选
- 📐 UI 缩放 50%–200%，DPI 自适应
- 🈲 中文字体支持（微软雅黑 / 宋体）

---

### 构建方法

**环境要求：** Windows · CMake 3.16+ · MinGW (i686/x86_64) 或 MSVC

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build . --config Release
```

产物为单文件 `tlmapp.exe`，静态链接，无需额外运行时。

---

## 🇺🇸 English Documentation

### Features

#### Transparency Control
- 🎚️ Per-window opacity adjustment (30%–100%)
- ⚡ Quick presets: 100% / 80% / 50% / 25%
- 🔄 One-click Hide/Show toggle for all effects

#### Color Overlay
- 🎨 Apply RGB color filters to any window
- 🌅 Preset modes: Normal / Warm / Cool / Night
- 🔧 Custom R/G/B + intensity adjustment

#### Window Locate
- 📍 Click the `+` button in title bar to enter locate mode
- 🖱️ Orange border overlay follows target window in real-time
- 📋 Left panel highlights synced selection
- ✅ Click to lock, ESC / right-click to cancel

#### Window Operations
- ⬆️ Pin to top / Minimize / Maximize / Restore / Close
- ✂️ Multi-select batch operations on multiple windows

#### Presets & Persistence
- 💾 Save/Restore complete window state snapshots (opacity + tint + topmost)
- 🧠 Auto-remember settings per exe + title

#### UI Customization
- 🎭 Three themes: Dark / Light / Glass
- 🔍 Search filter: real-time filtering by title or process name
- 📐 UI scale 50%–200%, DPI adaptive
- 🈲 Chinese font support (Microsoft YaHei / SimSun)

---

### Build Instructions

**Requirements:** Windows · CMake 3.16+ · MinGW (i686/x86_64) or MSVC

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build . --config Release
```

Output: single-file `tlmapp.exe`, statically linked, no runtime dependencies.

---

## 📸 Screenshots

| Main Interface | Color Overlay | Locate Mode |
|:--------------:|:-------------:|:-----------:|
| Window list with controls | RGB tint effects | Live window tracking |

---

## 🛠️ Tech Stack

| Component | Technology |
|-----------|------------|
| Language | C++17 |
| Renderer | DirectX 11 |
| UI Framework | Dear ImGui |
| Build System | CMake |
| Platform | Windows API |

---

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

---

<div align="center">

**Made with ❤️ by sykes**

</div>