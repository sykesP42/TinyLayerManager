#pragma once
#include <string>
#include "imgui.h"

struct Theme {
    ImVec4 bgColor;
    ImVec4 panelColor;
    ImVec4 cardColor;
    ImVec4 textColor;
    ImVec4 textSecondaryColor;
    ImVec4 accentColor;
    ImVec4 accentTextColor;
    ImVec4 borderColor;
    ImVec4 dangerColor;
    ImVec4 successColor;
    ImVec4 titleBarColor;

    void applyDark();
    void applyLight();
    void applyGlass();
    void applyToImGui();

    static std::string loadFromFile();
    static void saveToFile(const std::string& t);
    static Theme forTheme(const std::string& name);

    static const char* filePath();
};
