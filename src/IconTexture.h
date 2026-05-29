#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <windows.h>
#include <d3d9.h>

class IconTexture {
public:
    IconTexture(LPDIRECT3DDEVICE9 device);
    ~IconTexture();

    LPDIRECT3DTEXTURE9 get(const std::wstring& exePath, uint64_t hwnd);
    void clear();

private:
    LPDIRECT3DDEVICE9 m_device;
    struct TexEntry { LPDIRECT3DTEXTURE9 tex; };
    std::map<std::wstring, TexEntry> m_cache;

    static std::wstring makeKey(const std::wstring& exePath, uint64_t hwnd);
    LPDIRECT3DTEXTURE9 createFromHICON(HICON hIcon);
    LPDIRECT3DTEXTURE9 createFallback(const std::string& letter, float r, float g, float b);
};
