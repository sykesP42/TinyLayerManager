#include "WindowEnumerator.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <Psapi.h>
#endif

WindowEnumerator::WindowEnumerator() {}

WindowEnumerator::~WindowEnumerator() {
    if (m_thread.joinable())
        m_thread.join();
}

void WindowEnumerator::start(std::function<void(std::vector<WindowItem>)> onFinished) {
    if (m_running) return;
    m_running = true;
    m_callback = onFinished;
    if (m_thread.joinable())
        m_thread.detach(); // don't block the main thread
    m_thread = std::thread(&WindowEnumerator::threadFunc, this, onFinished);
}

void WindowEnumerator::threadFunc(std::function<void(std::vector<WindowItem>)> cb) {
    auto items = enumerateImpl();
    m_running = false;
    if (cb) cb(items);
}

#ifdef _WIN32
struct EnumData { std::vector<WindowItem>* out; };

static BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam) {
    EnumData* d = (EnumData*)lParam;
    bool visible = IsWindowVisible(hwnd);
    int len = GetWindowTextLengthW(hwnd);
    if (!visible || len == 0) return TRUE;

    std::wstring buf(len + 1, L'\0');
    GetWindowTextW(hwnd, &buf[0], len + 1);
    buf.resize(wcslen(buf.c_str()));

    wchar_t clsBuf[MAX_PATH] = {0};
    GetClassNameW(hwnd, clsBuf, MAX_PATH);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    std::wstring exePath;
    HANDLE ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (ph) {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameExW(ph, NULL, path, MAX_PATH))
            exePath = path;
        CloseHandle(ph);
    }

    WindowItem it;
    it.hwnd = (uint64_t)hwnd;
    it.title = buf;
    it.exe = exePath;
    it.className = clsBuf;
    it.pid = (uint32_t)pid;
    d->out->push_back(it);
    return TRUE;
}

std::vector<WindowItem> WindowEnumerator::enumerateImpl() {
    std::vector<WindowItem> items;
    EnumData d{&items};
    EnumWindows(EnumProc, (LPARAM)&d);
    return items;
}
#else
std::vector<WindowItem> WindowEnumerator::enumerateImpl() {
    return {};
}
#endif
