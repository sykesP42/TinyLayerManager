#pragma once
#include <string>
#include <vector>
#include <map>

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

class PresetManager {
public:
    PresetManager();

    void save(const std::string& name, const std::wstring& targetTitle,
              int alpha, bool top, int tintR, int tintG, int tintB, int tintIntensity);
    bool remove(const std::string& name);
    std::vector<std::string> names() const;
    Preset get(const std::string& name) const;

private:
    std::string m_filePath;
    std::map<std::string, Preset> m_presets;
    void load();
    void flush();
    static std::string dataPath();
};
