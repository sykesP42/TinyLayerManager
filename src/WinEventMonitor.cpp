#include "WinEventMonitor.h"
#include <cstdio>

WinEventMonitor* WinEventMonitor::s_instance = nullptr;

void CALLBACK WinEventMonitor::winEventProc(HWINEVENTHOOK, DWORD, HWND, LONG idObject,
                                             LONG, DWORD, DWORD){
    if (idObject != OBJID_WINDOW) return;
    if (!s_instance || !s_instance->m_notifyWindow) return;
    PostMessageW(s_instance->m_notifyWindow, WM_APP + 2, 0, 0);
}

WinEventMonitor::WinEventMonitor() {}

WinEventMonitor::~WinEventMonitor(){
    stop();
}

void WinEventMonitor::start(HWND notifyWindow){
    m_notifyWindow = notifyWindow;
    if (m_hook) return;
    s_instance = this;
    m_hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_NAMECHANGE,
                             NULL, winEventProc, 0, 0,
                             WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void WinEventMonitor::stop(){
    if (m_hook){
        UnhookWinEvent(m_hook);
        m_hook = NULL;
    }
    if (s_instance == this) s_instance = nullptr;
}
