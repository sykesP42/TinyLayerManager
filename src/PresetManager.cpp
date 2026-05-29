#include "PresetManager.h"
#include <cstdio>
#include <cstring>
#include <shlobj.h>

std::string PresetManager::dataPath() {
    wchar_t buf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    std::wstring dir = std::wstring(buf) + L"\\TLM";
    SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
    char mbuf[MAX_PATH];
    wcstombs(mbuf, dir.c_str(), MAX_PATH);
    return std::string(mbuf) + "\\presets.txt";
}

PresetManager::PresetManager() : m_filePath(dataPath()) {
    load();
}

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

bool PresetManager::remove(const std::string& name) {
    auto it = m_presets.find(name);
    if (it == m_presets.end()) return false;
    m_presets.erase(it);
    flush();
    return true;
}

std::vector<std::string> PresetManager::names() const {
    std::vector<std::string> out;
    for (auto& [k, v] : m_presets)
        out.push_back(k);
    return out;
}

Preset PresetManager::get(const std::string& name) const {
    auto it = m_presets.find(name);
    if (it != m_presets.end()) return it->second;
    return {};
}
