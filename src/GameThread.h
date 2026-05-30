#pragma once
#include <functional>

// Thread-safe queue for running tasks on the GTA SA game thread.
// All game object access (CPed, CVehicle, CStats, ...) MUST happen
// inside a callback posted through this module.
namespace GameThread {
    // Call once at plugin startup to subscribe to Events::gameProcessEvent.
    void init();

    // Post a task to be executed on the game thread on the next frame.
    // Thread-safe: may be called from any thread (io-thread, game-thread, etc.)
    void post(std::function<void()> fn);
}
