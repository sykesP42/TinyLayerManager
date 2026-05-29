# TLM UI Redesign — MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite the ImGui UI layer with proper layout, opacity presets, per-window RGB color tint, icon display, and collapsible sections.

**Architecture:** Keep existing ImGui + D3D11 framework. Extend PresetManager to hold full window-state snapshots. Add per-window settings persistence. Implement color tint as WS_EX_LAYERED overlay window per target. Extract window icons as D3D11 textures for ImGui::Image display.

**Tech Stack:** C++17, Dear ImGui 1.91, D3D11, Win32 API

---

### Task 1: Extend Preset — add top/tint fields

**Files:**
- Modify: `src/PresetManager.h`
- Modify: `src/PresetManager.cpp`

- [ ] **Step 1: Update Preset struct**

Edit `src/PresetManager.h`:

```cpp
struct Preset {
    std::string name;
    std::wstring targetTitle;
    int alpha = 255;
    bool top = false;
    int tintR = 255;
    int tintG = 255;
    int tintB = 255;
    int tintIntensity = 0;
};
```

- [ ] **Step 2: Update PresetManager::save signature**

```cpp
void save(const std::string& name, const std::wstring& targetTitle,
          int alpha, bool top, int tintR, int tintG, int tintB, int tintIntensity);
```

- [ ] **Step 3: Rewrite load() to parse new format**

New line format: `name|titleHex|alpha|top|tintR|tintG|tintB|tintIntensity`

Replace the `load()` body:

```cpp
void PresetManager::load() {
    m_presets.clear();
    FILE* f = fopen(m_filePath.c_str(), "r");
    if (!f) return;
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = 0;
        // name|titleHex|alpha|top|tintR|tintG|tintB|tintIntensity
        char* p1 = strchr(line, '|');
        if (!p1) continue; *p1 = 0;
        char* p2 = strchr(p1 + 1, '|');
        if (!p2) continue; *p2 = 0;
        char* p3 = strchr(p2 + 1, '|');
        if (!p3) continue; *p3 = 0;
        char* p4 = strchr(p3 + 1, '|');
        if (!p4) continue; *p4 = 0;
        char* p5 = strchr(p4 + 1, '|');
        if (!p5) continue; *p5 = 0;
        char* p6 = strchr(p5 + 1, '|');
        if (!p6) continue; *p6 = 0;
        char* p7 = strchr(p6 + 1, '|');
        if (!p7) continue; *p7 = 0;

        Preset pr;
        pr.name = line;
        // hex-decode title
        unsigned int titleLen = strlen(p1 + 1) / 2;
        if (titleLen > 0) {
            pr.targetTitle.resize(titleLen);
            for (unsigned int i = 0; i < titleLen; i++) {
                unsigned int c;
                sscanf(p1 + 1 + i * 2, "%02x", &c);
                pr.targetTitle[i] = (wchar_t)c;
            }
        }
        pr.alpha           = atoi(p2 + 1);
        pr.top             = atoi(p3 + 1) != 0;
        pr.tintR           = atoi(p4 + 1);
        pr.tintG           = atoi(p5 + 1);
        pr.tintB           = atoi(p6 + 1);
        pr.tintIntensity   = atoi(p7 + 1);
        m_presets[pr.name] = pr;
    }
    fclose(f);
}
```

- [ ] **Step 4: Rewrite flush() with new fields**

```cpp
void PresetManager::flush() {
    FILE* f = fopen(m_filePath.c_str(), "w");
    if (!f) return;
    for (auto& [name, pr] : m_presets) {
        fprintf(f, "%s|", name.c_str());
        for (wchar_t wc : pr.targetTitle)
            fprintf(f, "%02x", (unsigned int)(unsigned short)wc);
        fprintf(f, "|%d|%d|%d|%d|%d|%d\n",
                pr.alpha, pr.top ? 1 : 0,
                pr.tintR, pr.tintG, pr.tintB, pr.tintIntensity);
    }
    fclose(f);
}
```

- [ ] **Step 5: Update save() implementation**

```cpp
void PresetManager::save(const std::string& name, const std::wstring& targetTitle,
                         int alpha, bool top, int tintR, int tintG, int tintB, int tintIntensity) {
    Preset pr;
    pr.name = name;
    pr.targetTitle = targetTitle;
    pr.alpha = alpha;
    pr.top = top;
    pr.tintR = tintR;
    pr.tintG = tintG;
    pr.tintB = tintB;
    pr.tintIntensity = tintIntensity;
    m_presets[name] = pr;
    flush();
}
```

- [ ] **Step 6: Build and verify**

```bash
cd build_mingw64 && cmake --build . 2>&1 | tail -5
```

Expected: Build succeeds with no errors.

---

### Task 2: Add per-window settings persistence

**Files:**
- Create: `src/PerWindowSettings.h`
- Create: `src/PerWindowSettings.cpp`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Create PerWindowSettings.h**

```cpp
#pragma once
#include <string>
#include <map>
#include <cstdint>

struct PerWindowData {
    int alpha = 255;
    bool top = false;
    int tintR = 255;
    int tintG = 255;
    int tintB = 255;
    int tintIntensity = 0;
};

class PerWindowSettings {
public:
    PerWindowSettings();

    PerWindowData get(const std::wstring& exe, const std::wstring& title) const;
    void set(const std::wstring& exe, const std::wstring& title, const PerWindowData& d);

private:
    std::string m_filePath;
    std::map<std::wstring, PerWindowData> m_data; // key = exe\x0title
    void load();
    void flush();
    static std::string dataPath();
    static std::wstring key(const std::wstring& exe, const std::wstring& title);
};
```

- [ ] **Step 2: Create PerWindowSettings.cpp**

```cpp
#include "PerWindowSettings.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <shlobj.h>

std::string PerWindowSettings::dataPath() {
    wchar_t buf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    std::wstring dir = std::wstring(buf) + L"\\TLM";
    std::filesystem::create_directories(dir);
    char mbuf[MAX_PATH];
    wcstombs(mbuf, dir.c_str(), MAX_PATH);
    return std::string(mbuf) + "\\perwindow.txt";
}

std::wstring PerWindowSettings::key(const std::wstring& exe, const std::wstring& title) {
    return exe + L'\x0' + title;
}

PerWindowSettings::PerWindowSettings() : m_filePath(dataPath()) { load(); }

PerWindowData PerWindowSettings::get(const std::wstring& exe, const std::wstring& title) const {
    auto it = m_data.find(key(exe, title));
    if (it != m_data.end()) return it->second;
    return {};
}

void PerWindowSettings::set(const std::wstring& exe, const std::wstring& title, const PerWindowData& d) {
    m_data[key(exe, title)] = d;
    flush();
}

void PerWindowSettings::load() {
    m_data.clear();
    FILE* f = fopen(m_filePath.c_str(), "r");
    if (!f) return;
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = 0;
        // exeHex\x0titleHex|alpha|top|tintR|tintG|tintB|tintIntensity
        char* p1 = strchr(line, '|');
        if (!p1) continue; *p1 = 0;

        // key part is hex-encoded wide chars before the first |
        std::string keyHex = line;
        unsigned int keyLen = keyHex.size() / 2;
        std::wstring wkey;
        if (keyLen > 0) {
            wkey.resize(keyLen);
            for (unsigned int i = 0; i < keyLen; i++) {
                unsigned int c;
                sscanf(keyHex.c_str() + i * 2, "%02x", &c);
                wkey[i] = (wchar_t)c;
            }
        }

        PerWindowData d;
        char* p2 = strchr(p1 + 1, '|'); if (!p2) continue; *p2 = 0; d.alpha = atoi(p1 + 1);
        char* p3 = strchr(p2 + 1, '|'); if (!p3) continue; *p3 = 0; d.top = atoi(p2 + 1) != 0;
        char* p4 = strchr(p3 + 1, '|'); if (!p4) continue; *p4 = 0; d.tintR = atoi(p3 + 1);
        char* p5 = strchr(p4 + 1, '|'); if (!p5) continue; *p5 = 0; d.tintG = atoi(p4 + 1);
        char* p6 = strchr(p5 + 1, '|'); if (!p6) continue; *p6 = 0; d.tintB = atoi(p5 + 1);
        // tintIntensity is last
        d.tintIntensity = atoi(p6 + 1);

        m_data[wkey] = d;
    }
    fclose(f);
}

void PerWindowSettings::flush() {
    FILE* f = fopen(m_filePath.c_str(), "w");
    if (!f) return;
    for (auto& [k, d] : m_data) {
        for (wchar_t wc : k)
            fprintf(f, "%02x", (unsigned int)(unsigned short)wc);
        fprintf(f, "|%d|%d|%d|%d|%d|%d\n",
                d.alpha, d.top ? 1 : 0,
                d.tintR, d.tintG, d.tintB, d.tintIntensity);
    }
    fclose(f);
}
```

- [ ] **Step 3: Register in CMakeLists.txt**

Add to `src/CMakeLists.txt`:
```
PerWindowSettings.cpp
```

- [ ] **Step 4: Build verify**

```bash
cd build_mingw64 && cmake .. 2>&1 | tail -3 && cmake --build . 2>&1 | tail -5
```

Expected: Build succeeds.

---

### Task 3: Add icon texture extraction (HICON → D3D11 texture)

**Files:**
- Create: `src/IconTexture.h`
- Create: `src/IconTexture.cpp`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Create IconTexture.h**

```cpp
#pragma once
#include <string>
#include <map>
#include <windows.h>
#include <d3d11.h>

class IconTexture {
public:
    IconTexture(ID3D11Device* device);
    ~IconTexture();

    // Returns D3D11 shader-resource-view for the given exe path.
    // Caller must not Release() the returned pointer — owned by this class.
    ID3D11ShaderResourceView* get(const std::wstring& exePath, uint64_t hwnd);

    // Get a fallback texture for when extraction fails (16x16 colored square)
    ID3D11ShaderResourceView* fallback(const std::wstring& exePath);

    void clear();

private:
    ID3D11Device* m_device;
    struct TexEntry { ID3D11ShaderResourceView* srv; std::wstring key; };
    std::map<std::wstring, TexEntry> m_cache;

    static std::wstring makeKey(const std::wstring& exePath, uint64_t hwnd);
    ID3D11ShaderResourceView* createFromHICON(HICON hIcon);
    ID3D11ShaderResourceView* createFallback(const std::string& letter, float r, float g, float b);
};
```

- [ ] **Step 2: Create IconTexture.cpp**

```cpp
#include "IconTexture.h"
#include <shellapi.h>
#include <cstdio>

IconTexture::IconTexture(ID3D11Device* device) : m_device(device) {
    if (m_device) m_device->AddRef();
}

IconTexture::~IconTexture() {
    clear();
    if (m_device) m_device->Release();
}

std::wstring IconTexture::makeKey(const std::wstring& exePath, uint64_t hwnd) {
    if (!exePath.empty()) return exePath;
    return L"hwnd:" + std::to_wstring(hwnd);
}

ID3D11ShaderResourceView* IconTexture::get(const std::wstring& exePath, uint64_t hwnd) {
    auto k = makeKey(exePath, hwnd);
    auto it = m_cache.find(k);
    if (it != m_cache.end()) return it->second.srv;

    // Try ExtractIconEx first
    HICON hIcon = nullptr;
    bool needDestroy = false;

    if (!exePath.empty()) {
        wchar_t wpath[MAX_PATH];
        wcsncpy(wpath, exePath.c_str(), MAX_PATH - 1);
        wpath[MAX_PATH - 1] = 0;
        HICON big = nullptr, small = nullptr;
        UINT got = ExtractIconExW(wpath, 0, &big, &small, 1);
        if (got > 0) {
            hIcon = small ? small : big;
            needDestroy = true;
        }
    }

    // Try WM_GETICON from the window
    if (!hIcon && hwnd) {
        HWND h = (HWND)hwnd;
        hIcon = (HICON)SendMessageW(h, WM_GETICON, ICON_SMALL2, 0);
        if (!hIcon) hIcon = (HICON)SendMessageW(h, WM_GETICON, ICON_SMALL, 0);
        if (!hIcon) hIcon = (HICON)SendMessageW(h, WM_GETICON, ICON_BIG, 0);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(h, GCLP_HICON);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(h, GCLP_HICONSM);
    }

    ID3D11ShaderResourceView* srv = nullptr;
    if (hIcon) {
        srv = createFromHICON(hIcon);
        if (needDestroy) DestroyIcon(hIcon);
    }

    if (!srv) {
        // Build a colored fallback from the exe name's first letter
        std::string letter;
        if (!exePath.empty()) {
            auto pos = exePath.find_last_of(L'\\');
            std::wstring fname = (pos != std::wstring::npos) ? exePath.substr(pos + 1) : exePath;
            if (!fname.empty()) {
                char mb[8];
                wcstombs(mb, fname.c_str(), 8);
                letter = std::string(1, toupper(mb[0]));
            }
        }
        if (letter.empty()) letter = "?";
        // Deterministic color from the key
        int hash = 0;
        for (wchar_t wc : k) hash = hash * 31 + (int)wc;
        float r = ((hash >> 16) & 0xFF) / 255.0f;
        float g = ((hash >> 8) & 0xFF) / 255.0f;
        float b = (hash & 0xFF) / 255.0f;
        if (r < 0.3f) r = 0.3f; if (g < 0.3f) g = 0.3f; if (b < 0.3f) b = 0.3f;
        srv = createFallback(letter, r, g, b);
    }

    m_cache[k] = {srv, k};
    return srv;
}

ID3D11ShaderResourceView* IconTexture::createFromHICON(HICON hIcon) {
    if (!hIcon || !m_device) return nullptr;

    ICONINFO ii;
    if (!GetIconInfo(hIcon, &ii)) return nullptr;

    // Get icon dimensions from the bitmap
    BITMAP bm = {};
    if (ii.hbmColor) GetObject(ii.hbmColor, sizeof(bm), &bm);
    else if (ii.hbmMask) GetObject(ii.hbmMask, sizeof(bm), &bm);
    int w = bm.bmWidth, h = bm.bmHeight;
    if (w <= 0 || h <= 0) { if (ii.hbmColor) DeleteObject(ii.hbmColor); if (ii.hbmMask) DeleteObject(ii.hbmMask); return nullptr; }

    // Create a 32-bit BGRA texture
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DYNAMIC;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Texture2D* tex = nullptr;
    if (m_device->CreateTexture2D(&td, nullptr, &tex) != S_OK) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return nullptr;
    }

    // Draw icon to a DC, copy pixels
    HDC hdc = CreateCompatibleDC(nullptr);
    if (!hdc) { tex->Release(); if (ii.hbmColor) DeleteObject(ii.hbmColor); if (ii.hbmMask) DeleteObject(ii.hbmMask); return nullptr; }

    HBITMAP hbm = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ old = SelectObject(hdc, hbm);
    DrawIconEx(hdc, 0, 0, hIcon, w, h, 0, nullptr, DI_NORMAL);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (tex && m_device->GetImmediateContext()->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK) {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        GetDIBits(hdc, hbm, 0, h, mapped.pData, &bmi, DIB_RGB_COLORS);
        m_device->GetImmediateContext()->Unmap(tex, 0);
    }
    SelectObject(hdc, old);
    DeleteObject(hbm);
    DeleteDC(hdc);
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = td.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    if (m_device->CreateShaderResourceView(tex, &srvDesc, &srv) != S_OK)
        srv = nullptr;

    tex->Release();
    return srv;
}

ID3D11ShaderResourceView* IconTexture::createFallback(const std::string& letter, float r, float g, float b) {
    if (!m_device) return nullptr;

    int size = 16;
    uint32_t pixels[16 * 16];
    uint32_t bg = ((uint8_t)(r * 255) << 16) | ((uint8_t)(g * 255) << 8) | (uint8_t)(b * 255) | 0xFF000000;
    for (int i = 0; i < size * size; i++) pixels[i] = bg;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = size; td.Height = size; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = { pixels, size * 4, 0 };
    ID3D11Texture2D* tex = nullptr;
    if (m_device->CreateTexture2D(&td, &sd, &tex) != S_OK) return nullptr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = td.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    m_device->CreateShaderResourceView(tex, &srvDesc, &srv);
    tex->Release();
    return srv;
}

void IconTexture::clear() {
    for (auto& [k, v] : m_cache)
        if (v.srv) v.srv->Release();
    m_cache.clear();
}
```

- [ ] **Step 3: Register in CMakeLists.txt**

Add to `src/CMakeLists.txt`:
```
IconTexture.cpp
```

- [ ] **Step 4: Build verify**

```bash
cd build_mingw64 && cmake .. && cmake --build . 2>&1 | tail -5
```

Expected: Build succeeds.

---

### Task 4: Add tint overlay window management

**Files:**
- Modify: `src/WindowOperator.h`
- Modify: `src/WindowOperator.cpp`

- [ ] **Step 1: Add tint overlay methods to WindowOperator.h**

Add after existing static methods:

```cpp
class WindowOperator {
public:
    // ... existing methods ...

    // Color tint overlay management
    static void setWindowTint(HWND hWnd, int tintR, int tintG, int tintB, int tintIntensity);
    static void removeTint(HWND hWnd);
    static void updateAllOverlays(); // called every ~500ms
    static void cleanupAllOverlays();

    // ... existing private section ...
```

Also add private members:

```cpp
private:
    struct TintOverlay {
        HWND overlayHwnd = nullptr;
        int tintR = 255, tintG = 255, tintB = 255, tintIntensity = 0;
        uint64_t lastMoveTick = 0;
    };
    static std::map<HWND, TintOverlay> s_overlays;
    static void createOverlay(HWND hWnd, TintOverlay& ov);
    static void updateOverlay(HWND hWnd, TintOverlay& ov);
};
```

- [ ] **Step 2: Add static overlay map to .cpp**

At the top of `WindowOperator.cpp`, after includes:

```cpp
std::map<HWND, WindowOperator::TintOverlay> WindowOperator::s_overlays;
```

- [ ] **Step 3: Implement setWindowTint**

```cpp
void WindowOperator::setWindowTint(HWND hWnd, int tintR, int tintG, int tintB, int tintIntensity) {
    if (!hWnd) return;
    if (tintR == 255 && tintG == 255 && tintB == 255 && tintIntensity == 0) {
        removeTint(hWnd);
        return;
    }
    auto& ov = s_overlays[hWnd];
    ov.tintR = tintR;
    ov.tintG = tintG;
    ov.tintB = tintB;
    ov.tintIntensity = tintIntensity;
    if (!ov.overlayHwnd)
        createOverlay(hWnd, ov);
    else
        updateOverlay(hWnd, ov);
}
```

- [ ] **Step 4: Implement removeTint**

```cpp
void WindowOperator::removeTint(HWND hWnd) {
    auto it = s_overlays.find(hWnd);
    if (it == s_overlays.end()) return;
    if (it->second.overlayHwnd && IsWindow(it->second.overlayHwnd))
        DestroyWindow(it->second.overlayHwnd);
    s_overlays.erase(it);
}
```

- [ ] **Step 5: Implement createOverlay**

```cpp
void WindowOperator::createOverlay(HWND hWnd, TintOverlay& ov) {
    if (!hWnd) return;
    RECT rc;
    GetWindowRect(hWnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    ov.overlayHwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        L"TLM_TintOverlay", L"",
        WS_POPUP,
        rc.left, rc.top, w, h,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

    if (!ov.overlayHwnd) return;

    SetLayeredWindowAttributes(ov.overlayHwnd, 0, 0, LWA_ALPHA);
    ShowWindow(ov.overlayHwnd, SW_SHOWNA);
    updateOverlay(hWnd, ov);
}
```

- [ ] **Step 6: Implement updateOverlay**

```cpp
void WindowOperator::updateOverlay(HWND hWnd, TintOverlay& ov) {
    if (!ov.overlayHwnd || !IsWindow(ov.overlayHwnd) || !hWnd) return;

    // Reposition
    RECT rc;
    GetWindowRect(hWnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;
    SetWindowPos(ov.overlayHwnd, HWND_TOPMOST, rc.left, rc.top, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    // Calculate overlay color and alpha
    // Channel reduction: lower R/G/B value → more of that channel's complement in overlay
    BYTE or = (BYTE)(255 - ov.tintR);
    BYTE og = (BYTE)(255 - ov.tintG);
    BYTE ob = (BYTE)(255 - ov.tintB);
    BYTE oa = (BYTE)(ov.tintIntensity * 255 / 100);

    // Use a COLORREF for the overlay color
    HDC hdc = GetDC(ov.overlayHwnd);
    if (!hdc) return;
    HDC memdc = CreateCompatibleDC(hdc);
    if (!memdc) { ReleaseDC(ov.overlayHwnd, hdc); return; }

    // Create a 32bpp bitmap
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hbm = CreateDIBSection(memdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hbm) { DeleteDC(memdc); ReleaseDC(ov.overlayHwnd, hdc); return; }
    HGDIOBJ old = SelectObject(memdc, hbm);

    // Fill with RGBA color
    uint32_t color = (oa << 24) | (ob << 16) | (og << 8) | or;
    uint32_t* pixels = (uint32_t*)bits;
    for (int i = 0; i < w * h; i++) pixels[i] = color;

    SelectObject(memdc, old);

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;

    POINT ptZero = {0, 0};
    SIZE sz = {w, h};
    UpdateLayeredWindow(ov.overlayHwnd, hdc, nullptr, &sz, memdc, &ptZero, 0, &bf, ULW_ALPHA);

    DeleteObject(hbm);
    DeleteDC(memdc);
    ReleaseDC(ov.overlayHwnd, hdc);
}
```

- [ ] **Step 7: Implement updateAllOverlays / cleanupAllOverlays**

```cpp
void WindowOperator::updateAllOverlays() {
    std::vector<HWND> dead;
    for (auto& [hwnd, ov] : s_overlays) {
        if (!IsWindow(hwnd)) { dead.push_back(hwnd); continue; }
        updateOverlay(hwnd, ov);
    }
    for (HWND h : dead) removeTint(h);
}

void WindowOperator::cleanupAllOverlays() {
    for (auto& [hwnd, ov] : s_overlays) {
        if (ov.overlayHwnd && IsWindow(ov.overlayHwnd))
            DestroyWindow(ov.overlayHwnd);
    }
    s_overlays.clear();
}
```

- [ ] **Step 8: Register overlay window class in main.cpp**

Add before `RegisterClassExW(&wc)` in main.cpp:

```cpp
WNDCLASSEXW oc = {
    .cbSize        = sizeof(WNDCLASSEXW),
    .style         = CS_CLASSDC,
    .lpfnWndProc   = DefWindowProcW,
    .hInstance     = hInstance,
    .lpszClassName = L"TLM_TintOverlay"
};
RegisterClassExW(&oc);
```

- [ ] **Step 9: Call cleanup on exit**

Add before `eventMon.stop()` in main.cpp:

```cpp
WindowOperator::cleanupAllOverlays();
```

- [ ] **Step 10: Build verify**

```bash
cd build_mingw64 && cmake --build . 2>&1 | tail -10
```

Expected: Build succeeds.

---

### Task 5: Rewrite UI.cpp

**Files:**
- Modify: `src/UI.h`
- Modify: `src/UI.cpp`

This is the main UI rewrite. The entire right panel gets the collapsible-section layout. The left panel gets proper spacing and icon display.

- [ ] **Step 1: Update UI.h**

```cpp
#pragma once
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

void RenderUI(Theme& theme, std::string& themeName,
              std::vector<WindowItem>& windows, bool& shouldRefresh,
              WindowEnumerator& enumerator, WindowOperator& winOp,
              PresetManager& presetMgr, PerWindowSettings& perWin,
              IconTexture& iconTex, HWND hWnd);
```

- [ ] **Step 2: Write complete UI.cpp**

Replace `src/UI.cpp` with the implementation below.

```cpp
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600
#include <windows.h>

#include "UI.h"
#include "imgui.h"
#include "Theme.h"
#include "WindowItem.h"
#include "WindowEnumerator.h"
#include "WindowOperator.h"
#include "PresetManager.h"
#include "PerWindowSettings.h"
#include "IconTexture.h"

#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdint>

static std::string u8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), r.data(), n, nullptr, nullptr);
    return r;
}

// ── App state ──
static struct {
    int sel = -1;
    std::set<int> rows;
    char filter[256] = {};
    int alpha = 255;
    bool topChecked = false;
    // current window's tint values (before selection switch)
    int tintR = 255, tintG = 255, tintB = 255, tintIntensity = 0;
    char presetName[128] = {};
    uint64_t lastTick = 0;
    int closeTarget = -1;
    // track current exe/title to detect selection change
    std::wstring curExe, curTitle;
    // overlay update timer
    uint64_t lastOverlayTick = 0;
} s;

static bool match(const WindowItem& w, const char* f) {
    if (!f || !f[0]) return true;
    std::string ft(f); std::transform(ft.begin(), ft.end(), ft.begin(), ::tolower);
    auto t = u8(w.title); std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    if (t.find(ft) != std::string::npos) return true;
    auto e = u8(w.exe); std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    return e.find(ft) != std::string::npos;
}

void RenderUI(Theme& theme, std::string& themeName,
              std::vector<WindowItem>& windows, bool& shouldRefresh,
              WindowEnumerator& enumerator, WindowOperator& winOp,
              PresetManager& presetMgr, PerWindowSettings& perWin,
              IconTexture& iconTex, HWND hWnd)
{
    // ── Refresh logic ──
    if (shouldRefresh && !enumerator.isRunning()) {
        shouldRefresh = false;
        s.lastTick = GetTickCount64();
        enumerator.start([hWnd](auto items) {
            auto* p = new std::vector<WindowItem>(std::move(items));
            PostMessageW(hWnd, WM_APP + 1, 0, (LPARAM)p);
        });
    }
    uint64_t now = GetTickCount64();
    if (!enumerator.isRunning() && now - s.lastTick > 2000) {
        s.lastTick = now;
        enumerator.start([hWnd](auto items) {
            auto* p = new std::vector<WindowItem>(std::move(items));
            PostMessageW(hWnd, WM_APP + 1, 0, (LPARAM)p);
        });
    }

    // ── Overlay refresh every 500ms ──
    if (now - s.lastOverlayTick > 500) {
        s.lastOverlayTick = now;
        WindowOperator::updateAllOverlays();
    }

    // visible indices
    std::vector<int> vis;
    for (int i = 0; i < (int)windows.size(); i++)
        if (match(windows[i], s.filter)) vis.push_back(i);

    // ── Main window ──
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##Main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar);

    float ww = ImGui::GetContentRegionAvail().x;
    float wh = ImGui::GetContentRegionAvail().y;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    // ═══════════ TITLE BAR ═══════════
    const float th = 36;
    dl->AddRectFilled(origin, ImVec2(origin.x + ww, origin.y + th),
        ImGui::GetColorU32(theme.titleBarColor));

    ImGui::SetCursorScreenPos(ImVec2(origin.x + 12, origin.y + 10));
    ImGui::Text("TLM");

    ImGui::SetCursorScreenPos(origin);
    ImGui::InvisibleButton("##drag", ImVec2(ww - 135, th));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ReleaseCapture();
        SendMessageW(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        ImGui::ClearActiveID();
    }
    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
        ShowWindow(hWnd, IsZoomed(hWnd) ? SW_RESTORE : SW_MAXIMIZE);

    ImU32 colSec   = ImGui::GetColorU32(theme.textSecondaryColor);
    ImU32 colDanger = ImGui::GetColorU32(theme.dangerColor);
    ImU32 colHov   = ImGui::GetColorU32(ImVec4(1,1,1,0.1f));

    float bx = origin.x + ww;
    // close
    bx -= 38; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6));
    ImGui::InvisibleButton("##close", ImVec2(38, th-12));
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6), ImVec2(bx+38,origin.y+th-6), colDanger, 5);
    dl->AddText(ImVec2(bx+14, origin.y+11), ImGui::IsItemHovered() ? ImGui::GetColorU32(theme.bgColor) : colSec, "x");
    if (ImGui::IsItemClicked(0)) PostQuitMessage(0);
    // max
    bx -= 38; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6));
    ImGui::InvisibleButton("##max", ImVec2(38, th-12));
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6), ImVec2(bx+38,origin.y+th-6), colHov, 5);
    dl->AddText(ImVec2(bx+14, origin.y+11), colSec, IsZoomed(hWnd) ? "O" : "O");
    if (ImGui::IsItemClicked(0)) ShowWindow(hWnd, IsZoomed(hWnd) ? SW_RESTORE : SW_MAXIMIZE);
    // min
    bx -= 38; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6));
    ImGui::InvisibleButton("##min", ImVec2(38, th-12));
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6), ImVec2(bx+38,origin.y+th-6), colHov, 5);
    dl->AddText(ImVec2(bx+14, origin.y+11), colSec, "-");
    if (ImGui::IsItemClicked(0)) ShowWindow(hWnd, SW_MINIMIZE);
    // separator
    bx -= 5; dl->AddRectFilled(ImVec2(bx,origin.y+10), ImVec2(bx+1,origin.y+th-10), ImGui::GetColorU32(theme.borderColor));
    // theme
    bx -= 28; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6));
    ImGui::InvisibleButton("##theme", ImVec2(28, th-12));
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6), ImVec2(bx+28,origin.y+th-6), colHov, 5);
    dl->AddText(ImVec2(bx+7, origin.y+11), colSec, "@");
    if (ImGui::IsItemClicked(0)) ImGui::OpenPopup("##themeP");

    if (ImGui::BeginPopup("##themeP")) {
        const char* names[] = {"dark","light","glass"};
        const char* labels[] = {"Dark","Light","Glass"};
        for (int i = 0; i < 3; i++) {
            if (ImGui::Selectable(labels[i], themeName == names[i])) {
                themeName = names[i];
                theme = Theme::forTheme(themeName);
                theme.applyToImGui();
                Theme::saveToFile(themeName);
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + th));
    ImGui::Separator();

    // ═══════════ CONTENT ═══════════
    float contentY = ImGui::GetCursorPosY();
    float contentH = wh - contentY;
    static float leftW = 300;

    // ── LEFT PANEL ──
    ImGui::BeginChild("##left", ImVec2(leftW, contentH), false, ImGuiWindowFlags_NoScrollbar);

    // Search
    ImGui::SetCursorPos(ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
    ImGui::SetNextItemWidth(leftW - 16);
    ImGui::InputTextWithHint("##search", "Search...", s.filter, sizeof(s.filter));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // List
    ImGui::SetCursorPos(ImVec2(0, 40));
    float listH = contentH - 40 - 50;
    ImGui::BeginChild("##list", ImVec2(leftW, listH), false);

    if (vis.empty()) {
        ImGui::SetCursorPos(ImVec2(leftW/2 - 40, 20));
        ImGui::TextDisabled("No windows found");
    }

    float itemH = 30;
    char buf[256];

    for (size_t vi = 0; vi < vis.size(); vi++) {
        int ri = vis[vi];
        const auto& win = windows[ri];
        bool isSel = (s.sel == ri);
        bool isChk = s.rows.count(ri) > 0;

        ImGui::PushID(ri);
        ImVec2 a = ImGui::GetCursorScreenPos();
        ImVec2 b = ImVec2(a.x + leftW, a.y + itemH);

        if (isChk) dl->AddRectFilled(a, b, ImGui::GetColorU32(ImVec4(0.65f,0.92f,0.63f,0.15f)), 5);
        else if (isSel) dl->AddRectFilled(a, b, ImGui::GetColorU32(ImVec4(0.54f,0.71f,0.98f,0.15f)), 5);
        else if (ImGui::IsMouseHoveringRect(a, b)) dl->AddRectFilled(a, b, ImGui::GetColorU32(ImVec4(1,1,1,0.05f)), 5);

        // Checkbox
        ImGui::SetCursorPos(ImVec2(4, (itemH-14)/2));
        bool newChk = isChk;
        ImGui::Checkbox("##c", &newChk);
        if (newChk != isChk) { if (newChk) s.rows.insert(ri); else s.rows.erase(ri); }

        // Icon
        ImGui::SameLine(24);
        ID3D11ShaderResourceView* iconSrv = iconTex.get(win.exe, win.hwnd);
        if (iconSrv) {
            ImGui::Image((ImTextureID)(intptr_t)iconSrv, ImVec2(16, 16));
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
        } else {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 22);
        }

        // Title
        std::string t8 = u8(win.title);
        if (t8.empty()) t8 = "(untitled)";
        ImGui::SetCursorPosY((itemH - ImGui::GetFontSize()) / 2);
        ImGui::PushStyleColor(ImGuiCol_Text, isSel ? theme.textColor : theme.textSecondaryColor);
        ImGui::TextUnformatted(t8.c_str());
        ImGui::PopStyleColor();

        // Click
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##sel", ImVec2(leftW, itemH));
        if (ImGui::IsItemClicked(0)) {
            // Save previous window's settings
            if (s.sel >= 0 && s.sel < (int)windows.size()) {
                PerWindowData pd;
                pd.alpha = s.alpha; pd.top = s.topChecked;
                pd.tintR = s.tintR; pd.tintG = s.tintG; pd.tintB = s.tintB;
                pd.tintIntensity = s.tintIntensity;
                perWin.set(windows[s.sel].exe, windows[s.sel].title, pd);
            }
            // Load new selection
            s.sel = ri;
            auto pd = perWin.get(win.exe, win.title);
            s.alpha = pd.alpha; s.topChecked = pd.top;
            s.tintR = pd.tintR; s.tintG = pd.tintG; s.tintB = pd.tintB;
            s.tintIntensity = pd.tintIntensity;
            s.curExe = win.exe; s.curTitle = win.title;
            // Apply to window
            winOp.setWindowAlpha((HWND)win.hwnd, (unsigned char)s.alpha);
            winOp.setWindowTopmost((HWND)win.hwnd, s.topChecked);
            WindowOperator::setWindowTint((HWND)win.hwnd, s.tintR, s.tintG, s.tintB, s.tintIntensity);
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::PopID();
    }

    ImGui::EndChild();

    // Status
    ImGui::SetCursorPos(ImVec2(8, contentH - 44));
    snprintf(buf, sizeof(buf), "%zu windows", vis.size());
    ImGui::TextDisabled("%s", buf);
    if (s.rows.size() > 0) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.successColor);
        snprintf(buf, sizeof(buf), "(%zu)", s.rows.size());
        ImGui::TextUnformatted(buf);
        ImGui::PopStyleColor();
    }
    ImGui::SameLine(leftW - 75);
    if (vis.size() > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme.accentColor);
        if (ImGui::SmallButton("All")) { for (int i : vis) s.rows.insert(i); }
        ImGui::PopStyleColor();
    }
    if (s.rows.size() > 0) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.accentColor);
        if (ImGui::SmallButton("None")) s.rows.clear();
        ImGui::PopStyleColor();
    }

    // Batch bar
    if (s.rows.size() > 0) {
        float bbY = contentH - 28;
        ImGui::SetCursorPos(ImVec2(0, bbY));
        dl->AddRectFilled(ImGui::GetCursorScreenPos(),
            ImVec2(ImGui::GetCursorScreenPos().x + leftW, ImGui::GetCursorScreenPos().y + 28),
            ImGui::GetColorU32(theme.cardColor), 6);
        ImGui::SetCursorPos(ImVec2(6, bbY + 1));
        auto bb = [&](const char* lbl, const char* act, int val = 0) {
            if (ImGui::SmallButton(lbl)) {
                for (int i : s.rows) {
                    if (i >= 0 && i < (int)windows.size()) {
                        HWND h = (HWND)windows[i].hwnd; if (!h) continue;
                        if (!strcmp(act,"front")) { ShowWindow(h,SW_RESTORE); SetForegroundWindow(h); }
                        else if (!strcmp(act,"min")) winOp.minimizeWindow(h);
                        else if (!strcmp(act,"close")) PostMessageW(h,WM_CLOSE,0,0);
                        else if (!strcmp(act,"alpha")) winOp.setWindowAlpha(h,(unsigned char)val);
                        else if (!strcmp(act,"top")) winOp.setWindowTopmost(h,true);
                    }
                }
            }
            ImGui::SameLine();
        };
        bb("Front","front"); bb("Min","min");
        ImGui::PushStyleColor(ImGuiCol_Text, theme.dangerColor);
        bb("X","close"); ImGui::PopStyleColor();
        bb("50%","alpha",128); bb("80%","alpha",204); bb("100%","alpha",255); bb("T","top");
    }
    ImGui::EndChild();

    // ── SPLITTER ──
    ImGui::SameLine();
    ImVec2 sp = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(sp, ImVec2(sp.x+4, sp.y+contentH), ImGui::GetColorU32(theme.borderColor));
    ImGui::InvisibleButton("##split", ImVec2(4, contentH));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        leftW = std::max(220.0f, std::min(ww - 220.0f, leftW + ImGui::GetIO().MouseDelta.x));
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    // ── RIGHT PANEL ──
    ImGui::SameLine();
    ImGui::BeginChild("##right", ImVec2(0, contentH), false);

    if (s.sel < 0 || s.sel >= (int)windows.size()) {
        ImGui::SetCursorPos(ImVec2(20, 40));
        ImGui::TextDisabled("Select a window from the list");
    } else {
        const auto& win = windows[s.sel];
        float rw = ImGui::GetContentRegionAvail().x - 12;

        // ── Title + Icon ──
        ImGui::SetCursorPos(ImVec2(12, 10));
        ImGui::BeginGroup();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);

        ID3D11ShaderResourceView* iconSrv = iconTex.get(win.exe, win.hwnd);
        if (iconSrv)
            ImGui::Image((ImTextureID)(intptr_t)iconSrv, ImVec2(28, 28));
        else
            ImGui::Dummy(ImVec2(28, 28));
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6);

        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.textColor);
        std::string t8 = u8(win.title);
        ImGui::TextUnformatted(t8.c_str());
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.textSecondaryColor);
        ImGui::Text("%s", u8(win.exe).c_str());
        ImGui::PopStyleColor();
        ImGui::EndGroup();

        ImGui::EndGroup();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);

        // Details inline
        int px=0,py=0,pw=0,ph=0;
        WindowOperator::getWindowRect((HWND)win.hwnd, px,py,pw,ph);
        ImGui::PushStyleColor(ImGuiCol_Text, theme.textSecondaryColor);
        if (pw > 0) {
            snprintf(buf, sizeof(buf), "%d,%d  %dx%d  |  PID: %u", px, py, pw, ph, win.pid);
            ImGui::Text("%s", buf);
        } else {
            ImGui::Text("PID: %u", win.pid);
        }
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 6));

        // ── Segmented Action Buttons ──
        float bw = std::min(60.0f, (rw - 4*6) / 5);
        auto segBtn = [&](const char* label, bool danger, auto&& cb) {
            if (danger)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, theme.cardColor);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
            if (ImGui::Button(label, ImVec2(bw, 26))) cb();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::SameLine();
        };

        segBtn("Front", false, [&]{ HWND h=(HWND)win.hwnd; ShowWindow(h,SW_RESTORE); SetForegroundWindow(h); });
        segBtn("Min", false, [&]{ winOp.minimizeWindow((HWND)win.hwnd); });
        segBtn("Max", false, [&]{ winOp.maximizeWindow((HWND)win.hwnd); });
        segBtn("Restore", false, [&]{ winOp.restoreWindow((HWND)win.hwnd); });
        ImGui::PushStyleColor(ImGuiCol_Text, theme.dangerColor);
        segBtn("X", false, [&]{ s.closeTarget = s.sel; });
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 4));

        // ── Compact toggles row ──
        // Always on Top
        ImGui::PushStyleColor(ImGuiCol_Button, theme.cardColor);
        if (ImGui::Button(s.topChecked ? "Always on Top [ON]" : "Always on Top [OFF]", ImVec2(rw * 0.6f, 24))) {
            s.topChecked = !s.topChecked;
            winOp.setWindowTopmost((HWND)win.hwnd, s.topChecked);
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        // Reset All
        ImGui::PushStyleColor(ImGuiCol_Button, theme.cardColor);
        ImGui::PushStyleColor(ImGuiCol_Text, theme.dangerColor);
        if (ImGui::Button("Reset All", ImVec2(rw * 0.4f - 6, 24))) {
            for (auto& w : windows) {
                HWND h = (HWND)w.hwnd;
                if (h) { winOp.setWindowAlpha(h,255); winOp.setWindowTopmost(h,false);
                         WindowOperator::removeTint(h); }
            }
            s.alpha = 255; s.topChecked = false;
            s.tintR = 255; s.tintG = 255; s.tintB = 255; s.tintIntensity = 0;
        }
        ImGui::PopStyleColor(2);
        ImGui::Dummy(ImVec2(0, 2));

        // ── Collapsible: Opacity (default open) ──
        if (ImGui::CollapsingHeader("Opacity", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Preset buttons
            struct { const char* label; int val; } opts[] = {{"100%",255},{"80%",204},{"50%",128},{"25%",64}};
            for (auto& o : opts) {
                bool active = (s.alpha == o.val);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.54f,0.71f,0.98f,0.25f));
                else ImGui::PushStyleColor(ImGuiCol_Button, theme.cardColor);
                if (ImGui::SmallButton(o.label)) {
                    s.alpha = o.val;
                    winOp.setWindowAlpha((HWND)win.hwnd, (unsigned char)s.alpha);
                }
                ImGui::PopStyleColor();
                ImGui::SameLine();
            }
            ImGui::Dummy(ImVec2(0, 0)); // newline
            ImGui::SameLine(rw - 40);
            snprintf(buf, sizeof(buf), "%d%%", s.alpha * 100 / 255);
            ImGui::TextDisabled("%s", buf);

            // Slider
            ImGui::SetNextItemWidth(rw);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, theme.accentColor);
            if (ImGui::SliderInt("##alpha", &s.alpha, 30, 255, ""))
                winOp.setWindowAlpha((HWND)win.hwnd, (unsigned char)s.alpha);
            ImGui::PopStyleColor(2);
        }

        // ── Collapsible: Color Tint ──
        if (ImGui::CollapsingHeader("Color Tint", 0)) {
            // Built-in presets as chips
            struct TintPreset { const char* name; int r,g,b,intensity; };
            TintPreset tints[] = {
                {"Normal",255,255,255,0},
                {"Warm",255,220,180,30},
                {"Cool",200,220,255,30},
                {"Night",180,200,255,50},
            };
            for (auto& tp : tints) {
                bool isActive = (s.tintR == tp.r && s.tintG == tp.g && s.tintB == tp.b && s.tintIntensity == tp.intensity);
                if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, theme.accentColor);
                else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.2f,0.3f,0.5f));
                if (ImGui::SmallButton(tp.name)) {
                    s.tintR = tp.r; s.tintG = tp.g; s.tintB = tp.b; s.tintIntensity = tp.intensity;
                    WindowOperator::setWindowTint((HWND)win.hwnd, s.tintR, s.tintG, s.tintB, s.tintIntensity);
                }
                ImGui::PopStyleColor();
                ImGui::SameLine();
            }
            ImGui::Dummy(ImVec2(0, 0));
            ImGui::SameLine(rw - 30);
            if (ImGui::SmallButton("+ Save")) {
                // Save current tint as a named preset (reuse preset system)
                // prompt via next frame
            }

            // R/G/B sliders
            ImGui::Dummy(ImVec2(0, 2));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
            auto tintSlider = [&](const char* label, int* val, ImU32 col) {
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, col);
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, col);
                ImGui::SetNextItemWidth(rw - 20);
                if (ImGui::SliderInt(label, val, 0, 255, ""))
                    WindowOperator::setWindowTint((HWND)win.hwnd, s.tintR, s.tintG, s.tintB, s.tintIntensity);
                ImGui::PopStyleColor(2);
            };
            tintSlider("##R", &s.tintR, ImGui::GetColorU32(ImVec4(0.95f,0.54f,0.66f,1)));
            tintSlider("##G", &s.tintG, ImGui::GetColorU32(ImVec4(0.65f,0.89f,0.63f,1)));
            tintSlider("##B", &s.tintB, ImGui::GetColorU32(ImVec4(0.54f,0.71f,0.98f,1)));
            ImGui::PopStyleColor();

            // Intensity
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, theme.textColor);
            ImGui::SetNextItemWidth(rw - 20);
            if (ImGui::SliderInt("##I", &s.tintIntensity, 0, 100, "Intensity: %d%%"))
                WindowOperator::setWindowTint((HWND)win.hwnd, s.tintR, s.tintG, s.tintB, s.tintIntensity);
            ImGui::PopStyleColor(2);
        }

        // ── Collapsible: Saved Presets ──
        if (ImGui::CollapsingHeader("Saved Presets", 0)) {
            // Save row
            ImGui::SetNextItemWidth(rw - 30);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
            bool enterSave = ImGui::InputTextWithHint("##savep", "Save current as...",
                s.presetName, sizeof(s.presetName), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (ImGui::SmallButton("+") || enterSave) {
                std::string n(s.presetName);
                if (!n.empty()) {
                    presetMgr.save(n, win.title, s.alpha, s.topChecked,
                                   s.tintR, s.tintG, s.tintB, s.tintIntensity);
                    s.presetName[0] = 0;
                }
            }
            ImGui::PopStyleColor();

            // Preset list
            auto pnames = presetMgr.names();
            if (pnames.empty()) {
                ImGui::TextDisabled("No presets");
            } else {
                for (auto& nm : pnames) {
                    ImGui::PushID(nm.c_str());
                    bool hov = ImGui::IsMouseHoveringRect(
                        ImGui::GetCursorScreenPos(),
                        ImVec2(ImGui::GetCursorScreenPos().x + rw, ImGui::GetCursorScreenPos().y + 20));
                    if (hov) dl->AddRectFilled(ImGui::GetCursorScreenPos(),
                        ImVec2(ImGui::GetCursorScreenPos().x + rw, ImGui::GetCursorScreenPos().y + 20),
                        ImGui::GetColorU32(ImVec4(1,1,1,0.06f)), 3);
                    ImGui::TextDisabled("%s", nm.c_str());
                    if (ImGui::IsItemClicked(0)) {
                        auto pr = presetMgr.get(nm);
                        s.alpha = pr.alpha; s.topChecked = pr.top;
                        s.tintR = pr.tintR; s.tintG = pr.tintG; s.tintB = pr.tintB;
                        s.tintIntensity = pr.tintIntensity;
                        winOp.setWindowAlpha((HWND)win.hwnd, (unsigned char)s.alpha);
                        winOp.setWindowTopmost((HWND)win.hwnd, s.topChecked);
                        WindowOperator::setWindowTint((HWND)win.hwnd, s.tintR, s.tintG, s.tintB, s.tintIntensity);
                    }
                    ImGui::SameLine(rw - 20);
                    ImGui::PushStyleColor(ImGuiCol_Text, theme.dangerColor);
                    if (ImGui::SmallButton("x")) presetMgr.remove(nm);
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
            }
        }

        // ── Refresh ──
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Button, theme.cardColor);
        if (ImGui::Button("Refresh Window List", ImVec2(rw, 24))) shouldRefresh = true;
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::End();

    // ── Close confirm dialog ──
    if (s.closeTarget >= 0) {
        ImGui::OpenPopup("##cd");
        s.closeTarget = -1;
    }
    ImVec2 ctr = vp->GetCenter();
    ImGui::SetNextWindowPos(ctr, ImGuiCond_Appearing, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(260,120));
    if (ImGui::BeginPopupModal("##cd", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos(), p1(p0.x+260, p0.y+120);
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(theme.panelColor), 6);
        dl->AddRect(p0, p1, ImGui::GetColorU32(theme.borderColor), 6);
        bool ok = (s.sel >= 0 && s.sel < (int)windows.size());
        ImGui::SetCursorPos(ImVec2(20,30));
        ImGui::Text("Close this window?");
        ImGui::SetCursorPos(ImVec2(20,52));
        ImGui::TextDisabled("%s", ok ? u8(windows[s.sel].title).c_str() : "");
        ImGui::SetCursorPos(ImVec2(50,82));
        if (ImGui::Button("Cancel", ImVec2(70,26))) ImGui::CloseCurrentPopup();
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(70,26))) {
            if (ok) PostMessageW((HWND)windows[s.sel].hwnd, WM_CLOSE, 0, 0);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ── Resize handles ──
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    auto rEdge = [&](const char* id, float x, float y, float w, float h, int ht, ImGuiMouseCursor cur) {
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::InvisibleButton(id, ImVec2(w, h));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(cur);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            ReleaseCapture(); SendMessageW(hWnd, WM_NCLBUTTONDOWN, ht, 0); ImGui::ClearActiveID();
        }
    };
    int e = 4;
    rEdge("##RT",0,0,ds.x,e,HTTOP,ImGuiMouseCursor_ResizeNS);
    rEdge("##RB",0,ds.y-e,ds.x,e,HTBOTTOM,ImGuiMouseCursor_ResizeNS);
    rEdge("##RL",0,0,e,ds.y,HTLEFT,ImGuiMouseCursor_ResizeEW);
    rEdge("##RR",ds.x-e,0,e,ds.y,HTRIGHT,ImGuiMouseCursor_ResizeEW);
    rEdge("##RTL",0,0,12,12,HTTOPLEFT,ImGuiMouseCursor_ResizeNWSE);
    rEdge("##RTR",ds.x-12,0,12,12,HTTOPRIGHT,ImGuiMouseCursor_ResizeNESW);
    rEdge("##RBL",0,ds.y-12,12,12,HTBOTTOMLEFT,ImGuiMouseCursor_ResizeNESW);
    rEdge("##RBR",ds.x-12,ds.y-12,12,12,HTBOTTOMRIGHT,ImGuiMouseCursor_ResizeNWSE);
}
```

- [ ] **Step 3: Update RenderUI call in main.cpp**

Change line 222-224:
```cpp
RenderUI(currentTheme, themeName, g_windows, g_shouldRefresh,
         enumerator, winOp, presetMgr, g_hWnd);
```

To:
```cpp
RenderUI(currentTheme, themeName, g_windows, g_shouldRefresh,
         enumerator, winOp, presetMgr, perWinSettings, iconTex, g_hWnd);
```

And add to main.cpp variables section:
```cpp
PerWindowSettings perWinSettings;
IconTexture iconTex(g_pd3dDevice);
```

Also add includes at top of main.cpp:
```cpp
#include "PerWindowSettings.h"
#include "IconTexture.h"
```

- [ ] **Step 4: Add overlay window class registration**

In main.cpp, after `WNDCLASSEXW wc` block, add:

```cpp
WNDCLASSEXW oc = {
    .cbSize        = sizeof(WNDCLASSEXW),
    .style         = CS_CLASSDC,
    .lpfnWndProc   = DefWindowProcW,
    .hInstance     = hInstance,
    .lpszClassName = L"TLM_TintOverlay"
};
RegisterClassExW(&oc);
```

- [ ] **Step 5: Add overlay cleanup on exit**

In main.cpp, before `eventMon.stop();`:

```cpp
WindowOperator::cleanupAllOverlays();
```

- [ ] **Step 6: Build and test**

```bash
cd build_mingw64 && cmake .. && cmake --build . 2>&1 | tail -15
```

Expected: Build succeeds with no errors.

---

### Task 6: Final build and smoke test

- [ ] **Step 1: Clean rebuild**

```bash
cd build_mingw64 && cmake --build . --clean-first 2>&1 | tail -10
```

- [ ] **Step 2: Run app**

```bash
taskkill //F //IM tlmapp.exe 2>/dev/null; cp build_mingw64/src/tlmapp.exe /tmp/tlm-test2/ && start "" /tmp/tlm-test2/tlmapp.exe && sleep 3 && tasklist //FI "IMAGENAME eq tlmapp.exe"
```

- [ ] **Step 3: Verify no crash after 10 seconds**

```bash
sleep 10 && tasklist //FI "IMAGENAME eq tlmapp.exe"
```

- [ ] **Step 4: Kill app**

```bash
taskkill //F //IM tlmapp.exe
```
