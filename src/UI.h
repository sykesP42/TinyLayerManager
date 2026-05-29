#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

class Theme;
class WindowEnumerator;
class WindowOperator;
class PresetManager;
class PerWindowSettings;
class IconTexture;
struct WindowItem;

extern float g_dpiScale;
extern float g_zoomFactor;

void RenderUI(Theme& theme, std::string& themeName,
              std::vector<WindowItem>& windows, bool& shouldRefresh,
              WindowEnumerator& enumerator, WindowOperator& winOp,
              PresetManager& presetMgr, PerWindowSettings& perWin,
              IconTexture& iconTex, HWND hWnd);

// Locate mode (crosshair window picker)
bool& UILocateMode();
uint64_t& UILocateTarget();
