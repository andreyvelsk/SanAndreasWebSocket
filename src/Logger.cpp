// Logger.cpp
// All statics are POD types (bool, char[], CRITICAL_SECTION, HANDLE).
// None have non-trivial constructors → zero-initialized by the OS before
// any user code runs → no static initialization order fiasco.

#include "Logger.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>

namespace {
    bool             g_enabled  = false;          // POD, zero-init'd
    bool             g_csInit   = false;          // POD, zero-init'd
    char             g_path[MAX_PATH] = {};       // POD, zero-init'd
    CRITICAL_SECTION g_cs;                        // POD struct, zero-init'd
}

namespace Logger {

void init(bool enabled, const char* path)
{
    // Initialise critical section once (safe even if called twice —
    // re-init is benign for CRITICAL_SECTION on Windows).
    if (!g_csInit) {
        InitializeCriticalSection(&g_cs);
        g_csInit = true;
    }

    g_enabled = enabled;
    if (path) {
        strncpy_s(g_path, sizeof(g_path), path, _TRUNCATE);
    }

    if (!enabled || g_path[0] == '\0') return;

    // Create / truncate log file immediately so the user knows it worked.
    HANDLE h = CreateFileA(g_path,
                           GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        const char* hdr = "=== SanAndreasWebSocket trace log ===\r\n";
        DWORD written = 0;
        WriteFile(h, hdr, (DWORD)strlen(hdr), &written, NULL);
        CloseHandle(h);
    }
}

void trace(const char* fmt, ...)
{
    // Guard: g_enabled is true only after init() with enabled=true,
    // at which point g_csInit is also true and g_cs is valid.
    if (!g_enabled || g_path[0] == '\0') return;

    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, args);
    va_end(args);

    SYSTEMTIME st = {};
    GetLocalTime(&st);

    char line[1200];
    int len = _snprintf_s(line, sizeof(line), _TRUNCATE,
                          "[%02d:%02d:%02d.%03d] %s\r\n",
                          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                          msg);
    if (len < 0) len = (int)strlen(line);

    EnterCriticalSection(&g_cs);
    HANDLE h = CreateFileA(g_path,
                           GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        SetFilePointer(h, 0, NULL, FILE_END);
        DWORD written = 0;
        WriteFile(h, line, (DWORD)len, &written, NULL);
        CloseHandle(h);
    }
    LeaveCriticalSection(&g_cs);
}

} // namespace Logger
