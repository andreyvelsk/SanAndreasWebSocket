#pragma once

namespace Logger {
    // Call once, as early as possible (DLL constructor is fine).
    // Uses only POD internals — no static init order issues.
    // If enabled=false all trace() calls are no-ops (no file created).
    void init(bool enabled, const char* logFilePath);

    // Thread-safe (CRITICAL_SECTION). Writes timestamped line to log.
    // Safe to call before init() — will be a no-op.
    void trace(const char* fmt, ...);
}
