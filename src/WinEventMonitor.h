#pragma once
#include <windows.h>

class WinEventMonitor {
public:
    WinEventMonitor();
    ~WinEventMonitor();

    void start(HWND notifyWindow);
    void stop();

private:
    HWINEVENTHOOK m_hook = nullptr;
    HWND m_notifyWindow = nullptr;

    static WinEventMonitor* s_instance;
    static void CALLBACK winEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd,
                                      LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
};
