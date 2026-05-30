#pragma once
#include <string>

namespace Logger {
    // Call once at startup. If enabled=false, all trace() calls are no-ops.
    void init(bool enabled, const std::string& logFilePath);

    // Thread-safe. Writes timestamped line to log file if tracing is enabled.
    void trace(const char* fmt, ...);
}
