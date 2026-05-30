#include "GameThread.h"

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
            while (!g_queue.empty()) {
                auto fn = std::move(g_queue.front());
                g_queue.pop();
                lk.unlock();        // release lock while executing
                fn();
                lk.lock();
            }
        };
    }

    void post(std::function<void()> fn)
    {
        std::lock_guard lk(g_mutex);
        g_queue.push(std::move(fn));
    }

} // namespace GameThread
