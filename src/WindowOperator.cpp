#include "WindowOperator.h"
#include <map>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

bool WindowOperator::setAlphaByTitle(const std::wstring& title, unsigned char alpha) {
#ifdef _WIN32
    HWND h = findWindowByTitle(title);
    if (!h) return false;
    return setWindowAlpha(h, alpha);
#else
    (void)title; (void)alpha; return false;
#endif
}

bool WindowOperator::setWindowAlpha(HWND hWnd, unsigned char alpha) {
#ifdef _WIN32
    if (!hWnd) return false;
    LONG ex = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (!(ex & WS_EX_LAYERED))
        SetWindowLongPtrW(hWnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
    return SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA) != FALSE;
#else
    (void)hWnd; (void)alpha; return false;
#endif
}

bool WindowOperator::setWindowTopmost(HWND hWnd, bool on) {
#ifdef _WIN32
    if (!hWnd) return false;
    HWND insertAfter = on ? HWND_TOPMOST : HWND_NOTOPMOST;
    return SetWindowPos(hWnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE) != FALSE;
#else
    (void)hWnd; (void)on; return false;
#endif
}

bool WindowOperator::minimizeWindow(HWND hWnd) {
#ifdef _WIN32
    if (!hWnd) return false;
    return ShowWindow(hWnd, SW_MINIMIZE) != FALSE;
#else
    (void)hWnd; return false;
#endif
}

bool WindowOperator::maximizeWindow(HWND hWnd) {
#ifdef _WIN32
    if (!hWnd) return false;
    return ShowWindow(hWnd, SW_MAXIMIZE) != FALSE;
#else
    (void)hWnd; return false;
#endif
}

bool WindowOperator::restoreWindow(HWND hWnd) {
#ifdef _WIN32
    if (!hWnd) return false;
    return ShowWindow(hWnd, SW_RESTORE) != FALSE;
#else
    (void)hWnd; return false;
#endif
}

bool WindowOperator::getWindowRect(HWND hWnd, int& x, int& y, int& w, int& h) {
#ifdef _WIN32
    if (!hWnd) return false;
    RECT r;
    if (!GetWindowRect(hWnd, &r)) return false;
    x = r.left; y = r.top;
    w = r.right - r.left; h = r.bottom - r.top;
    return true;
#else
    (void)hWnd; (void)x; (void)y; (void)w; (void)h; return false;
#endif
}

#ifdef _WIN32
HBITMAP WindowOperator::captureWindowBitmap(HWND hWnd) {
    if (!hWnd) return NULL;
    RECT rc; GetWindowRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    HDC hdcWindow = GetWindowDC(hWnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, w, h);
    HGDIOBJ old = SelectObject(hdcMem, hbm);

    if (!PrintWindow(hWnd, hdcMem, PW_RENDERFULLCONTENT))
        BitBlt(hdcMem, 0, 0, w, h, hdcWindow, 0, 0, SRCCOPY);

    SelectObject(hdcMem, old);
    DeleteDC(hdcMem);
    ReleaseDC(hWnd, hdcWindow);
    return hbm;
}

struct FindData { std::wstring target; HWND result; };

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    FindData* d = (FindData*)lParam;
    int len = GetWindowTextLengthW(hwnd);
    if (len == 0) return TRUE;
    std::wstring buf(len + 1, L'\0');
    GetWindowTextW(hwnd, &buf[0], len + 1);
    buf.resize(wcslen(buf.c_str()));
    if (buf.find(d->target) != std::wstring::npos) {
        d->result = hwnd;
        return FALSE;
    }
    return TRUE;
}

HWND WindowOperator::findWindowByTitle(const std::wstring& title) {
    FindData data{ title, NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.result;
}

std::map<HWND, WindowOperator::TintOverlay> WindowOperator::s_overlays;
bool WindowOperator::s_liveRefresh = true;

void WindowOperator::setWindowTint(HWND hWnd, int tintR, int tintG, int tintB, int tintIntensity) {
    if (!hWnd || !IsWindow(hWnd)) return;
    if (tintIntensity == 0) {
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

void WindowOperator::removeTint(HWND hWnd) {
    auto it = s_overlays.find(hWnd);
    if (it == s_overlays.end()) return;
    if (it->second.overlayHwnd && IsWindow(it->second.overlayHwnd))
        DestroyWindow(it->second.overlayHwnd);
    if (it->second.bgBrush) DeleteObject(it->second.bgBrush);
    s_overlays.erase(it);
}

void WindowOperator::createOverlay(HWND hWnd, TintOverlay& ov) {
    if (!hWnd || !IsWindow(hWnd)) return;
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

    ShowWindow(ov.overlayHwnd, SW_SHOWNA);
    updateOverlay(hWnd, ov);
}

void WindowOperator::updateOverlay(HWND hWnd, TintOverlay& ov) {
    if (!ov.overlayHwnd || !IsWindow(ov.overlayHwnd) || !hWnd || !IsWindow(hWnd)) {
        if (ov.overlayHwnd && IsWindow(ov.overlayHwnd)) DestroyWindow(ov.overlayHwnd);
        ov.overlayHwnd = nullptr;
        return;
    }

    // Hide overlay when target is minimized, show when restored
    if (IsIconic(hWnd)) {
        ShowWindow(ov.overlayHwnd, SW_HIDE);
        return;
    }
    ShowWindow(ov.overlayHwnd, SW_SHOWNA);

    // Position overlay over target window
    RECT rc;
    GetWindowRect(hWnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;
    SetWindowPos(ov.overlayHwnd, HWND_TOPMOST, rc.left, rc.top, w, h,
                 SWP_NOACTIVATE | SWP_NOSENDCHANGING);

    // Set per-window alpha (0–255). Intensity 0 = invisible, 100 = fully opaque.
    BYTE alpha = (BYTE)(ov.tintIntensity * 255 / 100);
    SetLayeredWindowAttributes(ov.overlayHwnd, 0, alpha, LWA_ALPHA);

    // Update the tint colour: destroy old brush, create a new one with the
    // current RGB values and set it as the window-class background brush.
    // DefWindowProc's default WM_PAINT → BeginPaint → WM_ERASEBKGND will use
    // this brush to fill the entire client area. DWM then composites the
    // per-window alpha on top — giving us a solid-colour semi-transparent overlay.
    if (ov.bgBrush) DeleteObject(ov.bgBrush);
    ov.bgBrush = CreateSolidBrush(RGB(ov.tintB, ov.tintG, ov.tintR));
    SetClassLongPtrW(ov.overlayHwnd, GCLP_HBRBACKGROUND, (LONG_PTR)ov.bgBrush);

    // Trigger a repaint so DWM picks up the new background colour.
    RedrawWindow(ov.overlayHwnd, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_FRAME);
}

void WindowOperator::updateAllOverlays() {
    std::vector<HWND> dead;
    for (auto& [hwnd, ov] : s_overlays) {
        if (!IsWindow(hwnd)) { dead.push_back(hwnd); continue; }
        updateOverlay(hwnd, ov);
    }
    for (HWND h : dead) removeTint(h);
}

void WindowOperator::setAllOverlaysVisible(bool visible) {
    for (auto& [hwnd, ov] : s_overlays) {
        if (ov.overlayHwnd && IsWindow(ov.overlayHwnd))
            ShowWindow(ov.overlayHwnd, visible ? SW_SHOWNA : SW_HIDE);
    }
    // Re-position when showing again (target window may have moved)
    if (visible)
        updateAllOverlays();
}

void WindowOperator::cleanupAllOverlays() {
    for (auto& [hwnd, ov] : s_overlays) {
        if (ov.overlayHwnd && IsWindow(ov.overlayHwnd))
            DestroyWindow(ov.overlayHwnd);
        if (ov.bgBrush) DeleteObject(ov.bgBrush);
    }
    s_overlays.clear();
}

static BOOL CALLBACK DestroyOrphanedOverlayProc(HWND hwnd, LPARAM) {
    wchar_t cls[256];
    GetClassNameW(hwnd, cls, 256);
    if (wcscmp(cls, L"TLM_TintOverlay") == 0)
        DestroyWindow(hwnd);
    return TRUE;
}

void WindowOperator::destroyOrphanedOverlays() {
    EnumWindows(DestroyOrphanedOverlayProc, 0);
}
#endif
