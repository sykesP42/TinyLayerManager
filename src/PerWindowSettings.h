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
    std::map<std::wstring, PerWindowData> m_data;
    void load();
    void flush();
    static std::string dataPath();
    static std::wstring key(const std::wstring& exe, const std::wstring& title);
};
