#include "PerWindowSettings.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <shlobj.h>

std::string PerWindowSettings::dataPath() {
    wchar_t buf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    std::wstring dir = std::wstring(buf) + L"\\TLM";
    std::filesystem::create_directories(dir);
    char mbuf[MAX_PATH];
    wcstombs(mbuf, dir.c_str(), MAX_PATH);
    return std::string(mbuf) + "\\perwindow.txt";
}

std::wstring PerWindowSettings::key(const std::wstring& exe, const std::wstring& title) {
    return exe + L'\x0' + title;
}

PerWindowSettings::PerWindowSettings() : m_filePath(dataPath()) { load(); }

PerWindowData PerWindowSettings::get(const std::wstring& exe, const std::wstring& title) const {
    auto it = m_data.find(key(exe, title));
    if (it != m_data.end()) return it->second;
    return {};
}

void PerWindowSettings::set(const std::wstring& exe, const std::wstring& title, const PerWindowData& d) {
    m_data[key(exe, title)] = d;
    flush();
}

void PerWindowSettings::load() {
    m_data.clear();
    FILE* f = fopen(m_filePath.c_str(), "r");
    if (!f) return;
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = 0;
        // keyHex|alpha|top|tintR|tintG|tintB|tintIntensity
        char* p1 = strchr(line, '|');
        if (!p1) continue; *p1 = 0;

        // key part is hex-encoded wide chars before the first |
        std::string keyHex = line;
        unsigned int keyLen = keyHex.size() / 2;
        std::wstring wkey;
        if (keyLen > 0) {
            wkey.resize(keyLen);
            for (unsigned int i = 0; i < keyLen; i++) {
                unsigned int c;
                sscanf(keyHex.c_str() + i * 2, "%02x", &c);
                wkey[i] = (wchar_t)c;
            }
        }

        PerWindowData d;
        char* p2 = strchr(p1 + 1, '|'); if (!p2) continue; *p2 = 0; d.alpha = atoi(p1 + 1);
        char* p3 = strchr(p2 + 1, '|'); if (!p3) continue; *p3 = 0; d.top = atoi(p2 + 1) != 0;
        char* p4 = strchr(p3 + 1, '|'); if (!p4) continue; *p4 = 0; d.tintR = atoi(p3 + 1);
        char* p5 = strchr(p4 + 1, '|'); if (!p5) continue; *p5 = 0; d.tintG = atoi(p4 + 1);
        char* p6 = strchr(p5 + 1, '|'); if (!p6) continue; *p6 = 0; d.tintB = atoi(p5 + 1);
        d.tintIntensity = atoi(p6 + 1);

        m_data[wkey] = d;
    }
    fclose(f);
}

void PerWindowSettings::flush() {
    FILE* f = fopen(m_filePath.c_str(), "w");
    if (!f) return;
    for (auto& [k, d] : m_data) {
        for (wchar_t wc : k)
            fprintf(f, "%02x", (unsigned int)(unsigned short)wc);
        fprintf(f, "|%d|%d|%d|%d|%d|%d\n",
                d.alpha, d.top ? 1 : 0,
                d.tintR, d.tintG, d.tintB, d.tintIntensity);
    }
    fclose(f);
}
