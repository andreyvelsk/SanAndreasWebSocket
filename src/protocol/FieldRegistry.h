#pragma once
#include <string>
#include <nlohmann/json.hpp>

// Registry of readable game fields.
// ALL get() calls MUST be made from the game thread (inside GameThread::post).
namespace FieldRegistry {
    // Returns true if the field name is registered.
    bool has(const std::string& name);

    // Read current value of the named field.
    // Precondition: must be called on the game thread.
    // Returns nlohmann::json(nullptr) if the player is not available.
    nlohmann::json get(const std::string& name);

    // Register all built-in fields. Called once at startup.
    void init();
}
