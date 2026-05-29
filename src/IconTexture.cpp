#include "IconTexture.h"
#include <shellapi.h>
#include <cstdio>

IconTexture::IconTexture(ID3D11Device* device) : m_device(device) {
    if (m_device) m_device->AddRef();
}

IconTexture::~IconTexture() {
    clear();
    if (m_device) m_device->Release();
}

std::wstring IconTexture::makeKey(const std::wstring& exePath, uint64_t hwnd) {
    if (!exePath.empty()) return exePath;
    return L"hwnd:" + std::to_wstring(hwnd);
}

ID3D11ShaderResourceView* IconTexture::get(const std::wstring& exePath, uint64_t hwnd) {
    auto k = makeKey(exePath, hwnd);
    auto it = m_cache.find(k);
    if (it != m_cache.end()) return it->second.srv;

    HICON hIcon = nullptr;
    bool needDestroy = false;

    if (!exePath.empty()) {
        wchar_t wpath[MAX_PATH];
        wcsncpy(wpath, exePath.c_str(), MAX_PATH - 1);
        wpath[MAX_PATH - 1] = 0;
        HICON big = nullptr, small = nullptr;
        UINT got = ExtractIconExW(wpath, 0, &big, &small, 1);
        if (got > 0) {
            hIcon = small ? small : big;
            needDestroy = true;
        }
    }

    // Try SHGetFileInfo (handles most file types)
    if (!hIcon && !exePath.empty()) {
        SHFILEINFOW sfi = {};
        if (SHGetFileInfoW(exePath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON) && sfi.hIcon) {
            hIcon = sfi.hIcon;
            needDestroy = true;
        }
    }

    if (!hIcon && hwnd) {
        HWND h = (HWND)hwnd;
        DWORD_PTR dwResult = 0;
        if (SendMessageTimeoutW(h, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 200, &dwResult) && dwResult)
            hIcon = (HICON)dwResult;
        if (!hIcon) { dwResult = 0; if (SendMessageTimeoutW(h, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 200, &dwResult) && dwResult) hIcon = (HICON)dwResult; }
        if (!hIcon) { dwResult = 0; if (SendMessageTimeoutW(h, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 200, &dwResult) && dwResult) hIcon = (HICON)dwResult; }
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(h, GCLP_HICON);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(h, GCLP_HICONSM);
    }

    ID3D11ShaderResourceView* srv = nullptr;
    if (hIcon) {
        srv = createFromHICON(hIcon);
        if (needDestroy) DestroyIcon(hIcon);
    }

    if (!srv) {
        std::string letter;
        if (!exePath.empty()) {
            auto pos = exePath.find_last_of(L'\\');
            std::wstring fname = (pos != std::wstring::npos) ? exePath.substr(pos + 1) : exePath;
            if (!fname.empty()) {
                char mb[8] = {};
                wcstombs(mb, fname.c_str(), 8);
                letter = std::string(1, toupper((unsigned char)mb[0]));
            }
        }
        if (letter.empty()) letter = "?";
        int hash = 0;
        for (wchar_t wc : k) hash = hash * 31 + (int)wc;
        float r = ((hash >> 16) & 0xFF) / 255.0f;
        float g = ((hash >> 8) & 0xFF) / 255.0f;
        float b = (hash & 0xFF) / 255.0f;
        if (r < 0.3f) r = 0.3f; if (g < 0.3f) g = 0.3f; if (b < 0.3f) b = 0.3f;
        srv = createFallback(letter, r, g, b);
    }

    m_cache[k] = {srv};
    return srv;
}

ID3D11ShaderResourceView* IconTexture::createFromHICON(HICON hIcon) {
    if (!hIcon || !m_device) return nullptr;

    ICONINFO ii;
    if (!GetIconInfo(hIcon, &ii)) return nullptr;

    BITMAP bm = {};
    if (ii.hbmColor) GetObject(ii.hbmColor, sizeof(bm), &bm);
    else if (ii.hbmMask) GetObject(ii.hbmMask, sizeof(bm), &bm);
    int w = bm.bmWidth, h = bm.bmHeight;
    if (w <= 0 || h <= 0) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DYNAMIC;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Texture2D* tex = nullptr;
    if (m_device->CreateTexture2D(&td, nullptr, &tex) != S_OK) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return nullptr;
    }

    HDC hdc = CreateCompatibleDC(nullptr);
    if (!hdc) { tex->Release(); if (ii.hbmColor) DeleteObject(ii.hbmColor); if (ii.hbmMask) DeleteObject(ii.hbmMask); return nullptr; }

    // Use DIBSection so alpha channel is preserved (CreateCompatibleBitmap drops alpha)
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hbm) { DeleteDC(hdc); tex->Release(); if (ii.hbmColor) DeleteObject(ii.hbmColor); if (ii.hbmMask) DeleteObject(ii.hbmMask); return nullptr; }

    HGDIOBJ old = SelectObject(hdc, hbm);
    DrawIconEx(hdc, 0, 0, hIcon, w, h, 0, nullptr, DI_NORMAL);

    ID3D11DeviceContext* ctx = nullptr;
    m_device->GetImmediateContext(&ctx);
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (ctx && ctx->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK) {
        // DIBSection BGRA bytes → D3D11 B8G8R8A8 — direct match
        uint8_t* src = (uint8_t*)bits;
        uint8_t* dst = (uint8_t*)mapped.pData;
        for (int y = 0; y < h; y++) {
            memcpy(dst + y * mapped.RowPitch, src + y * (w * 4), w * 4);
        }
        ctx->Unmap(tex, 0);
    }
    if (ctx) ctx->Release();
    SelectObject(hdc, old);
    DeleteObject(hbm);
    DeleteDC(hdc);
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = td.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    if (m_device->CreateShaderResourceView(tex, &srvDesc, &srv) != S_OK)
        srv = nullptr;

    tex->Release();
    return srv;
}

ID3D11ShaderResourceView* IconTexture::createFallback(const std::string& letter, float r, float g, float b) {
    if (!m_device) return nullptr;

    int size = 16;
    uint32_t pixels[16 * 16];
    uint32_t bg = ((uint8_t)(r * 255) << 16) | ((uint8_t)(g * 255) << 8) | (uint8_t)(b * 255) | 0xFF000000u;
    for (int i = 0; i < size * size; i++) pixels[i] = bg;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = size; td.Height = size; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = { pixels, (UINT)(size * 4), 0 };
    ID3D11Texture2D* tex = nullptr;
    if (m_device->CreateTexture2D(&td, &sd, &tex) != S_OK) return nullptr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = td.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    m_device->CreateShaderResourceView(tex, &srvDesc, &srv);
    tex->Release();
    return srv;
}

void IconTexture::clear() {
    for (auto& [k, v] : m_cache)
        if (v.srv) v.srv->Release();
    m_cache.clear();
}
