#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <d3d11.h>
#include <vector>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "WindowItem.h"
#include "WindowEnumerator.h"
#include "WindowOperator.h"
#include "WinEventMonitor.h"
#include "Theme.h"
#include "PresetManager.h"
#include "PerWindowSettings.h"
#include "IconTexture.h"

// ── UI function ──
void RenderUI(Theme& theme, std::string& themeName,
              std::vector<WindowItem>& windows, bool& shouldRefresh,
              WindowEnumerator& enumerator, WindowOperator& winOp,
              PresetManager& presetMgr, PerWindowSettings& perWin,
              IconTexture& iconTex, HWND hWnd);

extern float g_dpiScale;

// ── D3D11 globals ──
static ID3D11Device*         g_pd3dDevice         = nullptr;
static ID3D11DeviceContext*  g_pd3dDeviceContext   = nullptr;
static IDXGISwapChain*       g_pSwapChain          = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static HWND g_hWnd = nullptr;
static bool g_running = true;

// ── Window list (written by WndProc on WM_APP+1, read by RenderUI) ──
static std::vector<WindowItem> g_windows;
static bool g_shouldRefresh = true;

// ── D3D11 helpers ──
static void CreateRenderTarget() {
    ID3D11Texture2D* back = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        g_pd3dDevice->CreateRenderTargetView(back, nullptr, &g_mainRenderTargetView);
        back->Release();
    }
}

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount       = 2;
    sd.BufferDesc.Width  = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow      = hWnd;
    sd.SampleDesc.Count  = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed          = TRUE;
    sd.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

    UINT createFlags         = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL levels[]{ D3D_FEATURE_LEVEL_11_0 };

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
            nullptr, createFlags, levels, 1, D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain        = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice        = nullptr; }
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
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0,
                (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
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
    SetProcessDPIAware();

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
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

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

        // Skip rendering when minimized — swap chain has 0-size buffers
        // and ImGui chokes on a 0x0 viewport.
        if (IsIconic(g_hWnd)) { WaitMessage(); continue; }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI(currentTheme, themeName, g_windows, g_shouldRefresh,
                 enumerator, winOp, presetMgr, perWinSettings, iconTex, g_hWnd);

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        float clear_color[4] = { 0 };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    WindowOperator::cleanupAllOverlays();
    eventMon.stop();
    DestroyWindow(g_hWnd);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    UnregisterClassW(L"TLM", hInstance);
    return 0;
}
