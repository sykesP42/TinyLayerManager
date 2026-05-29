#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include "WindowItem.h"

class WindowEnumerator {
public:
    WindowEnumerator();
    ~WindowEnumerator();

    void start(std::function<void(std::vector<WindowItem>)> onFinished);
    bool isRunning() const { return m_running; }

private:
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::function<void(std::vector<WindowItem>)> m_callback;

    void threadFunc(std::function<void(std::vector<WindowItem>)> cb);
    static std::vector<WindowItem> enumerateImpl();
};
