#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <d3d9.h>
#include <vector>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"

#include "WindowItem.h"
#include "WindowEnumerator.h"
#include "WindowOperator.h"
#include "WinEventMonitor.h"
#include "Theme.h"
#include "PresetManager.h"
#include "PerWindowSettings.h"
#include "IconTexture.h"
#include "UI.h"

extern float g_dpiScale;

// ── D3D9 globals ──
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

static HWND g_hWnd = nullptr;
static bool g_running = true;

// ── Window list (written by WndProc on WM_APP+1, read by RenderUI) ──
static std::vector<WindowItem> g_windows;
static bool g_shouldRefresh = true;

// ── D3D9 helpers ──
static bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed          = TRUE;
    g_d3dpp.SwapEffect        = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat  = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = FALSE;
    g_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

static void CleanupDeviceD3D() {
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D)       { g_pD3D->Release();       g_pD3D       = nullptr; }
}

static void ResetDevice() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3D_OK)
        ImGui_ImplDX9_CreateDeviceObjects();
}

// ── Window procedure ──
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        g_running = false;
        return 0;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_d3dpp.BackBufferWidth  = (UINT)LOWORD(lParam);
            g_d3dpp.BackBufferHeight = (UINT)HIWORD(lParam);
            if (g_pd3dDevice) ResetDevice();
        }
        return 0;

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
        return 0;

    case WM_CAPTURECHANGED:
        // ReleaseCapture() on drag/resize leaves ImGui with stuck MouseDown.
        // Reset here so clicks work again after window dragging.
        ImGui::GetIO().MouseDown[0] = false;
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR:
        if (UILocateMode()) {
            SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32515))); // IDC_CROSS
            return TRUE;
        }
        break;

    case WM_TIMER:
        if (wParam == 1 && UILocateMode()) {
            // Poll mouse state for locate mode
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                POINT pt; GetCursorPos(&pt);
                HWND target = WindowFromPoint(pt);
                if (target) target = GetAncestor(target, GA_ROOT);
                UILocateTarget() = (uint64_t)target;
                UILocateMode() = false;
                KillTimer(hWnd, 1);
                ShowWindow(hWnd, SW_RESTORE);
                SetForegroundWindow(hWnd);
            }
            else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000 ||
                     GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
                UILocateMode() = false;
                KillTimer(hWnd, 1);
                ShowWindow(hWnd, SW_RESTORE);
                SetForegroundWindow(hWnd);
            }
        }
        return 0;

    case WM_APP + 1: { // enumeration results from background thread
        auto* p = reinterpret_cast<std::vector<WindowItem>*>(lParam);
        if (p) { g_windows = std::move(*p); delete p; }
        return 0;
    }

    case WM_APP + 2: // window event, trigger refresh
        g_shouldRefresh = true;
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ── Entry point ──
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // DPI awareness: Vista+ has SetProcessDPIAware; on XP it's a no-op
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *PFN_SetProcessDPIAware)();
        auto pfn = (PFN_SetProcessDPIAware)GetProcAddress(hUser32, "SetProcessDPIAware");
        if (pfn) pfn();
    }

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconW(hInstance, L"IDI_TLM");
    wc.hIconSm       = LoadIconW(hInstance, L"IDI_TLM");
    wc.lpszClassName = L"TLM";
    RegisterClassExW(&wc);

    WNDCLASSEXW oc = {
        .cbSize        = sizeof(WNDCLASSEXW),
        .style         = CS_CLASSDC,
        .lpfnWndProc   = DefWindowProcW,
        .hInstance     = hInstance,
        .lpszClassName = L"TLM_TintOverlay"
    };
    RegisterClassExW(&oc);

    // Get system DPI for scaling
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);
    float dpiScale = dpi / 96.0f;

    int winW = (int)(780 * dpiScale), winH = (int)(460 * dpiScale);
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(WS_EX_APPWINDOW, L"TLM", L"TLM", WS_POPUP,
                             (scrW - winW) / 2, (scrH - winH) / 2, winW, winH,
                             nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd) { UnregisterClassW(L"TLM", hInstance); return 1; }

    if (!CreateDeviceD3D(g_hWnd)) {
        CleanupDeviceD3D(); DestroyWindow(g_hWnd);
        UnregisterClassW(L"TLM", hInstance); return 1;
    }

    // ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename  = nullptr;
    io.LogFilename  = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Load Chinese-capable font (scaled by DPI)
    {
        const wchar_t* candidates[] = {
            L"C:\\Windows\\Fonts\\msyh.ttc",   // Microsoft YaHei
            L"C:\\Windows\\Fonts\\simsun.ttc",  // SimSun
            L"C:\\Windows\\Fonts\\deng.ttf",    // DengXian
            L"C:\\Windows\\Fonts\\yahei.ttf",
        };
        ImFont* fnt = nullptr;
        float fontSize = 18.0f * dpiScale;
        for (auto p : candidates) {
            if (GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES) {
                char mb[260];
                wcstombs(mb, p, 260);
                fnt = io.Fonts->AddFontFromFileTTF(mb, fontSize, nullptr,
                    io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                if (fnt) break;
            }
        }
        if (!fnt)
            fnt = io.Fonts->AddFontDefault();
    }

    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Backend modules
    std::string   themeName = Theme::loadFromFile();
    Theme         currentTheme = Theme::forTheme(themeName);
    currentTheme.applyToImGui();

    // Scale ImGui style for DPI (after theme applies)
    ImGui::GetStyle().ScaleAllSizes(dpiScale);
    ImGui::GetIO().FontGlobalScale = 1.0f;
    g_dpiScale = dpiScale; // expose to UI.cpp

    WindowEnumerator enumerator;
    WindowOperator    winOp;
    WinEventMonitor   eventMon;
    PresetManager     presetMgr;
    PerWindowSettings perWinSettings;
    IconTexture       iconTex(g_pd3dDevice);
    eventMon.start(g_hWnd);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // Destroy orphaned tint overlays left by a previous crash (HWNDs survive process exit)
    WindowOperator::destroyOrphanedOverlays();

    // ── Main loop ──
    MSG msg{};
    while (g_running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!g_running) break;

        // Skip rendering when minimized
        if (IsIconic(g_hWnd)) { WaitMessage(); continue; }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI(currentTheme, themeName, g_windows, g_shouldRefresh,
                 enumerator, winOp, presetMgr, perWinSettings, iconTex, g_hWnd);

        ImGui::Render();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col = D3DCOLOR_RGBA(0, 0, 0, 0);
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET, clear_col, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT hr = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (hr == D3DERR_DEVICELOST)
            ResetDevice();
    }

    WindowOperator::cleanupAllOverlays();
    eventMon.stop();
    DestroyWindow(g_hWnd);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    UnregisterClassW(L"TLM", hInstance);
    return 0;
}
