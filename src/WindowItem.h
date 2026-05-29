#pragma once
#include <cstdint>
#include <string>

struct WindowItem {
    uint64_t hwnd = 0;
    std::wstring title;
    std::wstring exe;
    std::wstring className;
    uint32_t pid = 0;
};
