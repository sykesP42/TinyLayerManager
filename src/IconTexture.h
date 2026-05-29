#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <windows.h>
#include <d3d11.h>

class IconTexture {
public:
    IconTexture(ID3D11Device* device);
    ~IconTexture();

    ID3D11ShaderResourceView* get(const std::wstring& exePath, uint64_t hwnd);
    void clear();

private:
    ID3D11Device* m_device;
    struct TexEntry { ID3D11ShaderResourceView* srv; };
    std::map<std::wstring, TexEntry> m_cache;

    static std::wstring makeKey(const std::wstring& exePath, uint64_t hwnd);
    ID3D11ShaderResourceView* createFromHICON(HICON hIcon);
    ID3D11ShaderResourceView* createFallback(const std::string& letter, float r, float g, float b);
};
