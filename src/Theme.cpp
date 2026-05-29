#include "Theme.h"
#include <cstdio>
#include <shlobj.h>

static void ensureDir(const std::wstring& dir) {
    SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
}

static std::string dataDir() {
    wchar_t buf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    std::wstring dir = std::wstring(buf) + L"\\TLM";
    ensureDir(dir);
    char mbuf[MAX_PATH];
    wcstombs(mbuf, dir.c_str(), MAX_PATH);
    return std::string(mbuf);
}

const char* Theme::filePath() {
    static std::string path;
    if (path.empty())
        path = dataDir() + "\\theme.txt";
    return path.c_str();
}

std::string Theme::loadFromFile() {
    FILE* f = fopen(filePath(), "r");
    if (!f) return "dark";
    char buf[32] = {};
    if (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = 0;
        fclose(f);
        std::string s(buf);
        if (s == "light" || s == "glass") return s;
    }
    fclose(f);
    return "dark";
}

void Theme::saveToFile(const std::string& t) {
    FILE* f = fopen(filePath(), "w");
    if (!f) return;
    fputs(t.c_str(), f);
    fclose(f);
}

static ImVec4 hex(const char* c) {
    unsigned int r, g, b;
    sscanf(c, "%02x%02x%02x", &r, &g, &b);
    return ImVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f);
}

static ImVec4 rgba(int r, int g, int b, int a) {
    return ImVec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

void Theme::applyDark() {
    bgColor         = hex("1e1e2e");
    panelColor      = hex("181825");
    cardColor       = hex("313244");
    textColor       = hex("cdd6f4");
    textSecondaryColor = hex("6c7086");
    accentColor     = hex("89b4fa");
    accentTextColor = hex("1e1e2e");
    borderColor     = hex("45475a");
    dangerColor     = hex("f38ba8");
    successColor    = hex("a6e3a1");
    titleBarColor   = hex("181825");
}

void Theme::applyLight() {
    bgColor         = hex("f5f5f5");
    panelColor      = hex("ffffff");
    cardColor       = hex("f0f0f0");
    textColor       = hex("333333");
    textSecondaryColor = hex("888888");
    accentColor     = hex("4f8cff");
    accentTextColor = hex("ffffff");
    borderColor     = hex("e0e0e0");
    dangerColor     = hex("e53935");
    successColor    = hex("43a047");
    titleBarColor   = hex("ffffff");
}

void Theme::applyGlass() {
    bgColor         = rgba(20, 20, 40, 217);
    panelColor      = rgba(255, 255, 255, 20);
    cardColor       = rgba(255, 255, 255, 31);
    textColor       = hex("ffffff");
    textSecondaryColor = rgba(255, 255, 255, 153);
    accentColor     = hex("89b4fa");
    accentTextColor = hex("1e1e2e");
    borderColor     = rgba(255, 255, 255, 38);
    dangerColor     = hex("f38ba8");
    successColor    = hex("a6e3a1");
    titleBarColor   = rgba(20, 20, 40, 230);
}

Theme Theme::forTheme(const std::string& name) {
    Theme t;
    if (name == "light") t.applyLight();
    else if (name == "glass") t.applyGlass();
    else t.applyDark();
    return t;
}

void Theme::applyToImGui() {
    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg]          = bgColor;
    style.Colors[ImGuiCol_ChildBg]           = panelColor;
    style.Colors[ImGuiCol_Text]              = textColor;
    style.Colors[ImGuiCol_TextDisabled]      = textSecondaryColor;
    style.Colors[ImGuiCol_FrameBg]           = cardColor;
    style.Colors[ImGuiCol_FrameBgHovered]    = cardColor;
    style.Colors[ImGuiCol_FrameBgActive]     = cardColor;
    style.Colors[ImGuiCol_Border]            = borderColor;
    style.Colors[ImGuiCol_Button]            = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_ButtonHovered]     = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_ButtonActive]      = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_Header]            = panelColor;
    style.Colors[ImGuiCol_HeaderHovered]     = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_HeaderActive]      = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_ScrollbarBg]       = bgColor;
    style.Colors[ImGuiCol_ScrollbarGrab]     = borderColor;
    style.Colors[ImGuiCol_PopupBg]           = panelColor;
    style.Colors[ImGuiCol_ModalWindowDimBg]  = ImVec4(0,0,0,0.4f);

    style.FrameRounding    = 4.0f;
    style.GrabRounding     = 4.0f;
    style.PopupRounding    = 4.0f;
    style.ChildRounding    = 4.0f;
    style.ScrollbarSize    = 8.0f;
    style.FramePadding     = ImVec2(6, 3);
    style.ItemSpacing      = ImVec2(6, 3);
    style.ItemInnerSpacing = ImVec2(4, 2);
    style.WindowPadding    = ImVec2(0, 0);
    style.WindowBorderSize = 0;
}
