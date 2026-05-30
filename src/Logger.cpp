#include "Logger.h"

#include <windows.h>   // SYSTEMTIME, GetLocalTime
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <string>

namespace {
    bool        g_enabled = false;
    std::string g_path;
    std::mutex  g_mutex;
}

namespace Logger {

void init(bool enabled, const std::string& logFilePath)
{
    g_enabled = enabled;
    g_path    = logFilePath;

    if (!enabled) return;

    // Truncate / create the log file at startup
    FILE* f = nullptr;
    fopen_s(&f, g_path.c_str(), "w");
    if (f) {
        fprintf(f, "=== SanAndreasWebSocket trace log ===\n");
        fclose(f);
    }
}

void trace(const char* fmt, ...)
{
    if (!g_enabled) return;

    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, args);
    va_end(args);

    SYSTEMTIME st{};
    GetLocalTime(&st);

    char line[1200];
    snprintf(line, sizeof(line), "[%02d:%02d:%02d.%03d] %s\n",
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);

    std::lock_guard<std::mutex> lk(g_mutex);
    FILE* f = nullptr;
    fopen_s(&f, g_path.c_str(), "a");
    if (f) {
        fputs(line, f);
        fclose(f);
    }
}

} // namespace Logger
