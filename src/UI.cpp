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

float g_dpiScale = 1.0f;
float g_zoomFactor = 1.0f;

static std::string to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), r.data(), n, nullptr, nullptr);
    return r;
}

static struct {
    int sel = -1;
    std::set<int> rows;
    char filter[256] = {};
    int alpha = 255;
    bool topChecked = false;
    int tintR = 255;
    int tintG = 255;
    int tintB = 255;
    int tintIntensity = 0;
    char presetName[128] = {};
    uint64_t lastTick = 0;
    int closeTarget = -1;
    bool effectsHidden = false;
} g;

static void saveSettings(PerWindowSettings& perWin, const std::wstring& e, const std::wstring& t) {
    PerWindowData d;
    d.alpha = g.alpha;
    d.top = g.topChecked;
    d.tintR = g.tintR;
    d.tintG = g.tintG;
    d.tintB = g.tintB;
    d.tintIntensity = g.tintIntensity;
    perWin.set(e, t, d);
}

static void loadSettings(const PerWindowSettings& perWin, const std::wstring& e, const std::wstring& t) {
    PerWindowData d = perWin.get(e, t);
    g.alpha = d.alpha;
    g.topChecked = d.top;
    g.tintR = d.tintR;
    g.tintG = d.tintG;
    g.tintB = d.tintB;
    g.tintIntensity = d.tintIntensity;
}

static bool matchFilter(const WindowItem& w, const char* f) {
    if (!f || !f[0]) return true;
    std::string ft(f); std::transform(ft.begin(), ft.end(), ft.begin(), ::tolower);
    auto t = to_utf8(w.title); std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    if (t.find(ft) != std::string::npos) return true;
    auto e = to_utf8(w.exe); std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    return e.find(ft) != std::string::npos;
}

// ============================================================
void RenderUI(Theme& theme, std::string& themeName,
              std::vector<WindowItem>& windows, bool& shouldRefresh,
              WindowEnumerator& enumerator, WindowOperator& winOp,
              PresetManager& presetMgr, PerWindowSettings& perWin,
              IconTexture& iconTex, HWND hWnd)
{
    // ── Refresh ──
    if (shouldRefresh && !enumerator.isRunning()) {
        shouldRefresh = false;
        g.lastTick = GetTickCount64();
        enumerator.start([hWnd](std::vector<WindowItem> items) {
            auto* p = new std::vector<WindowItem>(std::move(items));
            PostMessageW(hWnd, WM_APP + 1, 0, (LPARAM)p);
        });
    }
    uint64_t now = GetTickCount64();
    if (!enumerator.isRunning() && now - g.lastTick > 2000) {
        g.lastTick = now;
        enumerator.start([hWnd](std::vector<WindowItem> items) {
            auto* p = new std::vector<WindowItem>(std::move(items));
            PostMessageW(hWnd, WM_APP + 1, 0, (LPARAM)p);
        });
    }

    // overlay auto-refresh every 100ms (position, minimize, restore)
    static uint64_t lastOverlayTick = 0;
    if (WindowOperator::s_liveRefresh && now - lastOverlayTick > 100) {
        lastOverlayTick = now;
        WindowOperator::updateAllOverlays();
    }

    // visible indices
    std::vector<int> vis;
    for (int i = 0; i < (int)windows.size(); i++)
        if (matchFilter(windows[i], g.filter)) vis.push_back(i);

    // ── Main window ──
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::Begin("##Main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    // Apply zoom delta to ImGui style and font (inside Begin, so "Debug" window is not current)
    static float prevZoom = 1.0f;
    if (g_zoomFactor != prevZoom) {
        float delta = g_zoomFactor / prevZoom;
        ImGui::GetStyle().ScaleAllSizes(delta);
        ImGui::GetIO().FontGlobalScale = g_zoomFactor;
        prevZoom = g_zoomFactor;
    }

    float S = g_dpiScale * g_zoomFactor;
    if (S < 0.5f) S = 1.0f; // sanity

    float ww = ImGui::GetContentRegionAvail().x;
    float wh = ImGui::GetContentRegionAvail().y;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    // =========================== TITLE BAR ===========================
    const float th = 40 * S;
    const float titleMidY = origin.y + th / 2;  // vertical center of title bar
    dl->AddRectFilled(origin, ImVec2(origin.x + ww, origin.y + th),
        ImGui::GetColorU32(theme.titleBarColor));

    // All left-side controls chain via SameLine for automatic spacing.
    float tlmY = titleMidY - ImGui::GetFontSize() / 2;
    ImGui::SetCursorScreenPos(ImVec2(origin.x + 12, tlmY));
    ImGui::Text("TLM");

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, theme.textSecondaryColor);
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    if (ImGui::SmallButton("-")) { g_zoomFactor = std::max(0.5f, g_zoomFactor - 0.1f); }
    ImGui::SameLine();
    ImGui::Text("%d%%", (int)(g_zoomFactor * 100));
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) { g_zoomFactor = std::min(2.0f, g_zoomFactor + 0.1f); }
    ImGui::PopStyleColor();

    // toolbar buttons: Hide/Show | Live | Reset
    ImGui::SameLine();
    ImGui::SetCursorPosY(tlmY);
    ImGui::PushStyleColor(ImGuiCol_Text, g.effectsHidden ? theme.textSecondaryColor : theme.accentColor);
    if (ImGui::SmallButton(g.effectsHidden ? "Show" : "Hide")) {
        g.effectsHidden = !g.effectsHidden;
        WindowOperator::setAllOverlaysVisible(!g.effectsHidden);
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, WindowOperator::s_liveRefresh ? theme.successColor : theme.textSecondaryColor);
    if (ImGui::SmallButton(WindowOperator::s_liveRefresh ? "Live" : "Off")) {
        WindowOperator::s_liveRefresh = !WindowOperator::s_liveRefresh;
        if (WindowOperator::s_liveRefresh)
            WindowOperator::updateAllOverlays();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, theme.dangerColor);
    if (ImGui::SmallButton("Reset")) {
        WindowOperator::cleanupAllOverlays();
        g.alpha = 255; g.topChecked = false;
        g.tintR = 255; g.tintG = 255; g.tintB = 255; g.tintIntensity = 0;
    }
    ImGui::PopStyleColor();

    // drag area — spans full title bar, buttons excluded on the right
    ImGui::SetCursorScreenPos(origin);
    ImGui::InvisibleButton("##drag", ImVec2(ww - 150 * S, th));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ReleaseCapture();
        SendMessageW(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        ImGui::GetIO().MouseDown[0] = false;
    }
    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
        ShowWindow(hWnd, IsZoomed(hWnd) ? SW_RESTORE : SW_MAXIMIZE);

    // buttons (right to left) — drawn via draw-list for font-independent sizing
    float bx = origin.x + ww;
    ImU32 colSec = ImGui::GetColorU32(theme.textSecondaryColor);
    ImU32 colDanger = ImGui::GetColorU32(theme.dangerColor);
    ImU32 colHov = ImGui::GetColorU32(ImVec4(1,1,1,0.1f));
    float btnPad = 6 * S;            // vertical padding inside each button
    ImVec2 btn38 = ImVec2(38 * S, th - 2 * btnPad);
    ImVec2 btn28 = ImVec2(28 * S, th - 2 * btnPad);
    // Modern minimal icons — geometric precision, uniform visual weight
    float lw = std::max(1.5f, 1.8f * S);

    auto drawIcon = [&](const char* type, bool hovered, float bx, float bw) {
        ImU32 col = (hovered && strcmp(type, "close") == 0) ? ImGui::GetColorU32(theme.bgColor) : colSec;
        float cx = bx + bw / 2;
        float cy = titleMidY;
        float s = bw * 0.26f;  // half-size
        float r = 2.0f * S;    // corner rounding

        if (strcmp(type, "close") == 0) {
            // ✕ diagonal cross
            dl->AddLine(ImVec2(cx - s, cy - s), ImVec2(cx + s, cy + s), col, lw);
            dl->AddLine(ImVec2(cx + s, cy - s), ImVec2(cx - s, cy + s), col, lw);
        }
        else if (strcmp(type, "max") == 0) {
            if (IsZoomed(hWnd)) {
                // Restore: back square offset, front square on top
                float o = s * 0.3f;
                dl->AddRect(ImVec2(cx - s + o, cy - s - o), ImVec2(cx + s + o, cy + s - o), col, r, 0, lw);
                dl->AddRectFilled(ImVec2(cx - s, cy - s), ImVec2(cx + s, cy + s), ImGui::GetColorU32(theme.titleBarColor), r);
                dl->AddRect(ImVec2(cx - s, cy - s), ImVec2(cx + s, cy + s), col, r, 0, lw);
            } else {
                dl->AddRect(ImVec2(cx - s, cy - s), ImVec2(cx + s, cy + s), col, r, 0, lw);
            }
        }
        else if (strcmp(type, "min") == 0) {
            // ─ short thick dash, slightly below centre like native title bars
            float y = cy + s * 0.15f;
            float lw2 = std::max(1.5f, 2.2f * S);
            dl->AddLine(ImVec2(cx - s, y), ImVec2(cx + s, y), col, lw2);
        }
        else if (strcmp(type, "theme") == 0) {
            // Three stacked dots (⋮ vertical ellipsis) — minimal, modern
            float dotR = std::max(1.0f, 1.5f * S);
            float gap = s * 0.5f;
            for (int i = -1; i <= 1; i++)
                dl->AddCircleFilled(ImVec2(cx, cy + i * gap), dotR, col);
        }
    };

    // close
    bx -= 38 * S; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6 * S));
    ImGui::InvisibleButton("##close", btn38);
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6*S), ImVec2(bx+38*S,origin.y+th-6*S), colDanger, 5);
    drawIcon("close", ImGui::IsItemHovered(), bx, 38*S);
    if (ImGui::IsItemClicked(0)) PostMessageW(hWnd, WM_CLOSE, 0, 0);

    // maximize
    bx -= 38 * S; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6 * S));
    ImGui::InvisibleButton("##max", btn38);
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6*S), ImVec2(bx+38*S,origin.y+th-6*S), colHov, 5);
    drawIcon("max", ImGui::IsItemHovered(), bx, 38*S);
    if (ImGui::IsItemClicked(0)) ShowWindow(hWnd, IsZoomed(hWnd) ? SW_RESTORE : SW_MAXIMIZE);

    // minimize
    bx -= 38 * S; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6 * S));
    ImGui::InvisibleButton("##min", btn38);
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6*S), ImVec2(bx+38*S,origin.y+th-6*S), colHov, 5);
    drawIcon("min", ImGui::IsItemHovered(), bx, 38*S);
    if (ImGui::IsItemClicked(0)) ShowWindow(hWnd, SW_MINIMIZE);

    // separator
    bx -= 5 * S; dl->AddRectFilled(ImVec2(bx,origin.y+10*S), ImVec2(bx+1,origin.y+th-10*S), ImGui::GetColorU32(theme.borderColor));

    // theme
    bx -= 28 * S; ImGui::SetCursorScreenPos(ImVec2(bx, origin.y + 6 * S));
    ImGui::InvisibleButton("##theme", btn28);
    if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(bx,origin.y+6*S), ImVec2(bx+28*S,origin.y+th-6*S), colHov, 5);
    drawIcon("theme", ImGui::IsItemHovered(), bx, 28*S);
    if (ImGui::IsItemClicked(0)) ImGui::OpenPopup("##themeP");

    // theme popup
    if (ImGui::BeginPopup("##themeP")) {
        const char* names[] = {"dark","light","glass"};
        const char* labels[] = {"Dark","Light","Glass"};
        for (int i = 0; i < 3; i++) {
            if (ImGui::Selectable(labels[i], themeName == names[i])) {
                themeName = names[i];
                theme = Theme::forTheme(themeName);
                theme.applyToImGui();
                Theme::saveToFile(themeName);
                prevZoom = 1.0f; // force re-apply zoom to new theme style
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + th));
    ImGui::Separator();

    // =========================== CONTENT ===========================
    float contentY = ImGui::GetCursorPosY();
    float contentH = wh - contentY;
    static float leftW = 320 * S;

    // ── LEFT PANEL ──
    ImGui::BeginChild("##left", ImVec2(leftW, contentH), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
    ImGui::SetNextItemWidth(leftW - 16);
    ImGui::InputTextWithHint("##search", "Search...", g.filter, sizeof(g.filter));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // Items area: scrollable list
    float itemH = 36 * S;
    float listY = 40;
    float listH = contentH - 40 - 50;

    if (vis.empty()) {
        ImGui::SetCursorPos(ImVec2(leftW/2 - 50, listY + 20));
        ImGui::TextDisabled("No windows found");
    }

    ImGui::SetCursorPos(ImVec2(0, listY));
    ImGui::BeginChild("##list", ImVec2(leftW, listH), false);
    ImDrawList* dl2 = ImGui::GetWindowDrawList();

    for (size_t vi = 0; vi < vis.size(); vi++) {
        int ri = vis[vi];
        const auto& win = windows[ri];
        bool isSel = (g.sel == ri);
        bool isChk = g.rows.count(ri) > 0;

        ImGui::PushID(ri);
        ImVec2 itemPos = ImGui::GetCursorScreenPos();
        float ix = itemPos.x, iy = itemPos.y;

        ImGui::InvisibleButton("##sel", ImVec2(leftW, itemH));
        if (ImGui::IsItemClicked(0)) {
            if (g.sel >= 0 && g.sel < (int)windows.size())
                saveSettings(perWin, windows[g.sel].exe, windows[g.sel].title);
            g.sel = ri;
            loadSettings(perWin, win.exe, win.title);
            HWND h = (HWND)win.hwnd;
            if (h) {
                winOp.setWindowAlpha(h, (unsigned char)g.alpha);
                winOp.setWindowTopmost(h, g.topChecked);
                winOp.setWindowTint(h, g.tintR, g.tintG, g.tintB, g.tintIntensity);
            }
        }
        bool hovered = ImGui::IsItemHovered();

        ImU32 bgCol;
        if (isChk) bgCol = ImGui::GetColorU32(ImVec4(0.65f,0.92f,0.63f,0.15f));
        else if (isSel) bgCol = ImGui::GetColorU32(ImVec4(0.54f,0.71f,0.98f,0.15f));
        else if (hovered) bgCol = ImGui::GetColorU32(ImVec4(1,1,1,0.05f));
        else bgCol = 0;
        if (bgCol) dl2->AddRectFilled(ImVec2(ix,iy), ImVec2(ix+leftW, iy+itemH), bgCol, 5);

        // checkbox (scale with DPI)
        float cz = 14 * S;
        float cbx = ix + 4 * S, cby = iy + (itemH - cz) / 2;
        ImVec2 cbMin(cbx,cby), cbMax(cbx+cz,cby+cz);
        if (isChk) {
            dl2->AddRectFilled(cbMin, cbMax, ImGui::GetColorU32(theme.accentColor), 3);
            dl2->AddText(ImVec2(cbx+cz*0.2f,cby+cz*0.1f), ImGui::GetColorU32(theme.accentTextColor), "v");
        } else {
            dl2->AddRect(cbMin, cbMax, ImGui::GetColorU32(theme.borderColor), 3);
        }
        if (ImGui::IsMouseHoveringRect(cbMin, cbMax) && ImGui::IsMouseClicked(0)) {
            if (isChk) g.rows.erase(ri); else g.rows.insert(ri);
        }

        // icon
        float iconS = 16 * S;
        float iconX = ix + 24 * S, iconY = iy + (itemH - iconS) / 2;
        ID3D11ShaderResourceView* srv = iconTex.get(win.exe, win.hwnd);
        if (srv)
            dl2->AddImage((ImTextureID)(intptr_t)srv, ImVec2(iconX,iconY), ImVec2(iconX+iconS,iconY+iconS));
        else
            dl2->AddRectFilled(ImVec2(iconX,iconY), ImVec2(iconX+iconS,iconY+iconS),
                ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.4f,0.5f)), 3);

        // title
        std::string t8 = to_utf8(win.title);
        if (t8.empty()) t8 = "(untitled)";
        float tx = iconX + 20 * S, ty = iy + (itemH - ImGui::GetFontSize()) / 2;
        dl2->AddText(ImVec2(tx, ty),
            ImGui::GetColorU32(isSel ? theme.textColor : theme.textSecondaryColor),
            t8.c_str());

        ImGui::PopID();
    }

    ImGui::EndChild(); // list

    // status
    ImGui::SetCursorPos(ImVec2(8, contentH - 44));
    char buf[64];
    snprintf(buf, sizeof(buf), "%zu windows", vis.size());
    ImGui::TextDisabled("%s", buf);
    if (g.rows.size() > 0) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.successColor);
        snprintf(buf, sizeof(buf), "(%zu)", g.rows.size());
        ImGui::TextUnformatted(buf);
        ImGui::PopStyleColor();
    }
    ImGui::SameLine(leftW - 75);
    if (vis.size() > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme.accentColor);
        if (ImGui::SmallButton("All")) { for (int i : vis) g.rows.insert(i); }
        ImGui::PopStyleColor();
    }
    if (g.rows.size() > 0) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.accentColor);
        if (ImGui::SmallButton("None")) g.rows.clear();
        ImGui::PopStyleColor();
    }

    // batch bar
    if (g.rows.size() > 0) {
        float bbY = contentH - 28;
        ImGui::SetCursorPos(ImVec2(0, bbY));
        dl->AddRectFilled(ImGui::GetCursorScreenPos(),
            ImVec2(ImGui::GetCursorScreenPos().x + leftW, ImGui::GetCursorScreenPos().y + 28),
            ImGui::GetColorU32(theme.cardColor), 6);
        ImGui::SetCursorPos(ImVec2(6, bbY + 1));

        auto bb = [&](const char* lbl, const char* act, int val = 0) {
            if (ImGui::SmallButton(lbl)) {
                for (int i : g.rows) {
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
    ImGui::EndChild(); // left

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

    if (g.sel < 0 || g.sel >= (int)windows.size()) {
        ImGui::SetCursorPos(ImVec2(20, 40));
        ImGui::TextDisabled("Select a window from the list");
    } else {
        const auto& win = windows[g.sel];
        float rw = ImGui::GetContentRegionAvail().x - 18;

        // ── Title section: icon + title + exe path ──
        ImGui::SetCursorPos(ImVec2(12, 10));
        {
            ID3D11ShaderResourceView* srv = iconTex.get(win.exe, win.hwnd);
            if (srv) {
                ImGui::Image((ImTextureID)(intptr_t)srv, ImVec2(24, 24));
                ImGui::SameLine();
                ImGui::SetCursorPosY(10 + (24 - ImGui::GetFontSize()) / 2);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, theme.textColor);
            ImGui::TextUnformatted(to_utf8(win.title).c_str());
            ImGui::PopStyleColor();
        }

        // exe path
        ImGui::PushStyleColor(ImGuiCol_Text, theme.textSecondaryColor);
        ImGui::TextUnformatted(to_utf8(win.exe).c_str());
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 4));

        // ── Details: position + PID ──
        int px=0,py=0,pw=0,ph=0;
        WindowOperator::getWindowRect((HWND)win.hwnd, px,py,pw,ph);
        if (pw > 0) {
            snprintf(buf, sizeof(buf), "%d,%d  %dx%d", px, py, pw, ph);
            ImGui::PushStyleColor(ImGuiCol_Text, theme.accentColor);
            ImGui::TextUnformatted(buf);
            ImGui::PopStyleColor();
        }
        ImGui::PushStyleColor(ImGuiCol_Text, theme.textSecondaryColor);
        ImGui::Text("PID: %u", win.pid);
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 4));

        // ── Segmented action buttons ──
        float bw = std::min(65.0f, (rw - 4*6) / 5);
        auto aBtn = [&](const char* label, bool danger, auto&& cb) {
            if (danger) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            else ImGui::PushStyleColor(ImGuiCol_Button, theme.cardColor);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
            if (ImGui::Button(label, ImVec2(bw, 28))) cb();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::SameLine();
        };

        aBtn("Front", false, [&]{ HWND h=(HWND)win.hwnd; ShowWindow(h,SW_RESTORE); SetForegroundWindow(h); });
        aBtn("Min", false, [&]{ winOp.minimizeWindow((HWND)win.hwnd); });
        aBtn("Max", false, [&]{ winOp.maximizeWindow((HWND)win.hwnd); });
        aBtn("Restore", false, [&]{ winOp.restoreWindow((HWND)win.hwnd); });
        ImGui::PushStyleColor(ImGuiCol_Text, theme.dangerColor);
        aBtn("X", false, [&]{ g.closeTarget = g.sel; });
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 4));

        // ── Compact row: Always on Top + Reset All ──
        float halfW = (rw - 4) / 2;
        ImGui::PushStyleColor(ImGuiCol_Button, g.topChecked ? ImVec4(0.65f,0.92f,0.63f,0.15f) : theme.cardColor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
        if (ImGui::Button(g.topChecked ? "Always on Top [ON]" : "Always on Top [OFF]", ImVec2(halfW, 26))) {
            g.topChecked = !g.topChecked;
            winOp.setWindowTopmost((HWND)win.hwnd, g.topChecked);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
        if (ImGui::Button("Reset All", ImVec2(halfW, 26))) {
            winOp.setWindowAlpha((HWND)win.hwnd, 255);
            winOp.setWindowTopmost((HWND)win.hwnd, false);
            winOp.removeTint((HWND)win.hwnd);
            g.alpha = 255; g.topChecked = false;
            g.tintR = 255; g.tintG = 255; g.tintB = 255; g.tintIntensity = 0;
        }
        ImGui::PopStyleVar();
        ImGui::Dummy(ImVec2(0, 6));

        // ── CollapsingHeader "Opacity" (default open) ──
        if (ImGui::CollapsingHeader("Opacity", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(8.0f);

            // 4 preset buttons
            const char* alphaLabels[] = {"100%", "80%", "50%", "25%"};
            int alphaVals[] = {255, 204, 128, 64};
            for (int i = 0; i < 4; i++) {
                if (ImGui::SmallButton(alphaLabels[i])) {
                    g.alpha = alphaVals[i];
                    winOp.setWindowAlpha((HWND)win.hwnd, (unsigned char)g.alpha);
                }
                if (i < 3) ImGui::SameLine();
            }

            // percentage label
            snprintf(buf, sizeof(buf), "%d%%", g.alpha * 100 / 255);
            float labelW = ImGui::CalcTextSize(buf).x;
            ImGui::SameLine(rw - 12 - 8 - labelW);
            ImGui::TextUnformatted(buf);

            // fine slider
            ImGui::SetNextItemWidth(rw - 12 - 8);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, theme.accentColor);
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, theme.accentColor);
            if (ImGui::SliderInt("##alpha", &g.alpha, 30, 255, ""))
                winOp.setWindowAlpha((HWND)win.hwnd, (unsigned char)g.alpha);
            ImGui::PopStyleColor(3);

            ImGui::Unindent(8.0f);
        }

        // ── CollapsingHeader "Color Tint" (collapsed by default) ──
        if (ImGui::CollapsingHeader("Color Tint")) {
            ImGui::Indent(8.0f);

            // 4 preset chips
            const char* tintLabels[] = {"Normal", "Warm", "Cool", "Night"};
            int tintRPreset[] = {255, 255, 180, 255};
            int tintGPreset[] = {255, 200, 210, 150};
            int tintBPreset[] = {255, 150, 255, 100};
            int tintIPreset[] = {0, 30, 25, 50};
            for (int i = 0; i < 4; i++) {
                if (ImGui::SmallButton(tintLabels[i])) {
                    g.tintR = tintRPreset[i];
                    g.tintG = tintGPreset[i];
                    g.tintB = tintBPreset[i];
                    g.tintIntensity = tintIPreset[i];
                    winOp.setWindowTint((HWND)win.hwnd, g.tintR, g.tintG, g.tintB, g.tintIntensity);
                }
                if (i < 3) ImGui::SameLine();
            }

            // R slider (red grab)
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1,0,0,1));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1,0.3f,0.3f,1));
            ImGui::SetNextItemWidth(rw - 12 - 8);
            if (ImGui::SliderInt("R", &g.tintR, 0, 255))
                winOp.setWindowTint((HWND)win.hwnd, g.tintR, g.tintG, g.tintB, g.tintIntensity);
            ImGui::PopStyleColor(2);

            // G slider (green grab)
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0,1,0,1));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.3f,1,0.3f,1));
            ImGui::SetNextItemWidth(rw - 12 - 8);
            if (ImGui::SliderInt("G", &g.tintG, 0, 255))
                winOp.setWindowTint((HWND)win.hwnd, g.tintR, g.tintG, g.tintB, g.tintIntensity);
            ImGui::PopStyleColor(2);

            // B slider (blue grab)
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0,0,1,1));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.3f,0.3f,1,1));
            ImGui::SetNextItemWidth(rw - 12 - 8);
            if (ImGui::SliderInt("B", &g.tintB, 0, 255))
                winOp.setWindowTint((HWND)win.hwnd, g.tintR, g.tintG, g.tintB, g.tintIntensity);
            ImGui::PopStyleColor(2);

            // Intensity slider (0-100%)
            ImGui::SetNextItemWidth(rw - 12 - 8);
            if (ImGui::SliderInt("Intensity", &g.tintIntensity, 0, 100, "%d%%"))
                winOp.setWindowTint((HWND)win.hwnd, g.tintR, g.tintG, g.tintB, g.tintIntensity);

            ImGui::Unindent(8.0f);
        }

        // ── CollapsingHeader "Saved Presets" (collapsed by default) ──
        if (ImGui::CollapsingHeader("Saved Presets")) {
            ImGui::Indent(8.0f);

            // Input + save button
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.cardColor);
            float inputW = rw - 12 - 8 - 30;
            ImGui::SetNextItemWidth(inputW);
            bool enterSave = ImGui::InputTextWithHint("##preset", "Save preset...", g.presetName, sizeof(g.presetName), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (ImGui::SmallButton("+") || enterSave) {
                std::string n(g.presetName);
                if (!n.empty()) {
                    presetMgr.save(n, win.title, g.alpha, g.topChecked,
                                   g.tintR, g.tintG, g.tintB, g.tintIntensity);
                    g.presetName[0] = 0;
                }
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            // Preset list
            auto pnames = presetMgr.names();
            if (pnames.empty()) {
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::TextDisabled("No presets");
            } else {
                for (auto& nm : pnames) {
                    ImGui::PushID(nm.c_str());
                    ImVec2 pa = ImGui::GetCursorScreenPos();
                    float itemRw = rw - 12 - 8;
                    bool hov = ImGui::IsMouseHoveringRect(pa, ImVec2(pa.x + itemRw, pa.y + 20));
                    if (hov) dl->AddRectFilled(pa, ImVec2(pa.x + itemRw, pa.y + 20),
                        ImGui::GetColorU32(ImVec4(1,1,1,0.06f)), 3);
                    ImGui::TextDisabled("%s", nm.c_str());
                    if (ImGui::IsItemClicked(0)) {
                        auto pr = presetMgr.get(nm);
                        if (pr.alpha > 0) {
                            g.alpha = pr.alpha;
                            g.topChecked = pr.top;
                            g.tintR = pr.tintR;
                            g.tintG = pr.tintG;
                            g.tintB = pr.tintB;
                            g.tintIntensity = pr.tintIntensity;
                            // Apply all at once
                            HWND h = (HWND)win.hwnd;
                            if (h) {
                                winOp.setWindowAlpha(h, (unsigned char)g.alpha);
                                winOp.setWindowTopmost(h, g.topChecked);
                                winOp.setWindowTint(h, g.tintR, g.tintG, g.tintB, g.tintIntensity);
                            }
                        }
                    }
                    ImGui::SameLine(itemRw - 20);
                    ImGui::PushStyleColor(ImGuiCol_Text, theme.dangerColor);
                    if (ImGui::SmallButton("x")) presetMgr.remove(nm);
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
            }
            ImGui::Unindent(8.0f);
        }

        // ── Refresh button ──
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Button, theme.cardColor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
        if (ImGui::Button("Refresh Window List", ImVec2(rw, 26))) shouldRefresh = true;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    ImGui::EndChild(); // right
    ImGui::PopStyleVar(); // padding
    ImGui::End(); // main

    // ── Close confirm dialog ──
    if (g.closeTarget >= 0) {
        ImGui::OpenPopup("##cd");
        g.closeTarget = -1;
    }
    ImVec2 ctr = vp->GetCenter();
    ImGui::SetNextWindowPos(ctr, ImGuiCond_Appearing, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(260,120));
    if (ImGui::BeginPopupModal("##cd", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos(), p1(p0.x+260, p0.y+120);
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(theme.panelColor), 6);
        dl->AddRect(p0, p1, ImGui::GetColorU32(theme.borderColor), 6);
        bool ok = (g.sel >= 0 && g.sel < (int)windows.size());
        ImGui::SetCursorPos(ImVec2(20,30));
        ImGui::Text("Close this window?");
        ImGui::SetCursorPos(ImVec2(20,52));
        ImGui::TextDisabled("%s", ok ? to_utf8(windows[g.sel].title).c_str() : "");
        ImGui::SetCursorPos(ImVec2(50,82));
        if (ImGui::Button("Cancel", ImVec2(70,26))) ImGui::CloseCurrentPopup();
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(70,26))) {
            if (ok) PostMessageW((HWND)windows[g.sel].hwnd, WM_CLOSE, 0, 0);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ── Resize handles ──
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    // Own overlay to avoid activating ImGui's internal "Debug" fallback window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ds);
    ImGui::Begin("##resizeOverlay", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing);
    auto rEdge = [&](const char* id, float x, float y, float w, float h, int ht, ImGuiMouseCursor cur) {
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::InvisibleButton(id, ImVec2(w, h));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(cur);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            ReleaseCapture(); SendMessageW(hWnd, WM_NCLBUTTONDOWN, ht, 0);
            ImGui::GetIO().MouseDown[0] = false;
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
    ImGui::End(); // resizeOverlay
}
