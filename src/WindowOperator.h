#pragma once
#include <string>
#include <cstdint>
#include <map>

#ifdef _WIN32
#include <windows.h>
#endif

class WindowOperator {
public:
    bool setAlphaByTitle(const std::wstring& title, unsigned char alpha);

    static bool setWindowAlpha(HWND hWnd, unsigned char alpha);
    static bool setWindowTopmost(HWND hWnd, bool on);
    static bool minimizeWindow(HWND hWnd);
    static bool maximizeWindow(HWND hWnd);
    static bool restoreWindow(HWND hWnd);
    static bool getWindowRect(HWND hWnd, int& x, int& y, int& w, int& h);

#ifdef _WIN32
    static HBITMAP captureWindowBitmap(HWND hWnd);
#endif

    // Color tint overlay management
    static void setWindowTint(HWND hWnd, int tintR, int tintG, int tintB, int tintIntensity);
    static void removeTint(HWND hWnd);
    static void updateAllOverlays();
    static void cleanupAllOverlays();
    static void destroyOrphanedOverlays();
    static bool s_liveRefresh;
    static bool s_overlaysVisible;
    static void setAllOverlaysVisible(bool visible);

private:
    struct TintOverlay {
        HWND overlayHwnd = nullptr;
        int tintR = 255, tintG = 255, tintB = 255, tintIntensity = 0;
        HBRUSH bgBrush = nullptr; // cached background brush, destroyed on update
    };
    static std::map<HWND, TintOverlay> s_overlays;
    static std::map<HWND, unsigned char> s_alphaMap; // track per-window alpha for hide/show
    static void createOverlay(HWND hWnd, TintOverlay& ov);
    static void updateOverlay(HWND hWnd, TintOverlay& ov);

#ifdef _WIN32
    HWND findWindowByTitle(const std::wstring& title);
#endif
};
