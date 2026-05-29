# TLM — Tiny Layer Manager

轻量级 Windows 窗口管理工具。控制透明度、色彩叠加、窗口状态预设。

## 功能

- 窗口透明度 — 实时调节单窗口不透明度（30%–100%），支持快捷预设
- 色彩叠加 — 为任意窗口应用 RGB 色彩滤镜（暖色/冷色/夜间模式/自定义）
- 窗口操作 — 置顶、最小化、最大化、还原、关闭
- 预设系统 — 保存/恢复完整的窗口状态快照
- 多选批操作 — 同时对多个窗口执行统一操作
- 搜索过滤 — 按窗口标题或进程名实时筛选
- 主题切换 — 深色 / 浅色 / 玻璃 三套 UI 主题
- 自动持久化 — 按 exe+标题自动记忆窗口设置

## 构建

**环境要求：** Windows 10+ · CMake 3.16+ · MinGW 或 MSVC

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

编译产物为单文件 `tlmapp.exe`，无需额外依赖。

## License

See [LICENSE](LICENSE).
