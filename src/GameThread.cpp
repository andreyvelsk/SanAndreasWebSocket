#include "GameThread.h"
#include "Logger.h"

#include "plugin.h"

#include <mutex>
#include <queue>

namespace {
    std::queue<std::function<void()>> g_queue;
    std::mutex                        g_mutex;
}

namespace GameThread {

    void init()
    {
        plugin::Events::gameProcessEvent += []() {
            std::unique_lock lk(g_mutex);
            if (g_queue.empty()) return;
            int count = 0;
            while (!g_queue.empty()) {
                auto fn = std::move(g_queue.front());
                g_queue.pop();
                ++count;
                lk.unlock();
                fn();
                lk.lock();
            }
            Logger::trace("GameThread: drained %d task(s)", count);
        };
    }

    void post(std::function<void()> fn)
    {
        Logger::trace("GameThread: post() called");
        std::lock_guard lk(g_mutex);
        g_queue.push(std::move(fn));
    }

} // namespace GameThread
