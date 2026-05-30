#include "server/WsServer.h"
#include "GameThread.h"
#include "Logger.h"
#include "protocol/FieldRegistry.h"

#include "plugin.h"
#include "extensions/FontPrint.h"

#include <boost/asio.hpp>
#include <memory>
#include <thread>

namespace asio = boost::asio;
using     tcp  = asio::ip::tcp;

// ---------- вспомогательные функции пути (только стек + WinAPI) -------------
// Возвращает путь к файлу рядом с gta_sa.exe.
// Не использует std::string — безопасно вызывать на DLL-load.
static void buildSidePath(char* out, size_t outSz, const char* filename)
{
    char proc[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, proc, MAX_PATH);
    char* slash = strrchr(proc, '\\');
    if (slash) {
        size_t base = (size_t)(slash + 1 - proc);
        strncpy_s(out, outSz, proc, base);
        strncat_s(out, outSz, filename, _TRUNCATE);
    } else {
        strncpy_s(out, outSz, filename, _TRUNCATE);
    }
}
// ----------------------------------------------------------------------------

static asio::io_context                                              g_ioc;
static std::unique_ptr<WsServer>                                     g_server;
static std::thread                                                   g_ioThread;
static std::unique_ptr<asio::executor_work_guard<
    asio::io_context::executor_type>>                                g_workGuard;

static char g_bindStr[72] = "?";   // показывается на экране (host:port)

using namespace plugin;

static struct SanAndreasWebSocket {
    SanAndreasWebSocket() {
        // ─── ШАГИ 1-3: инициализируем Logger как можно раньше ───────────────
        // Logger использует только POD-типы (char[], bool, CRITICAL_SECTION) —
        // не зависит от порядка инициализации других TU-static'ов.
        {
            char iniPath[MAX_PATH] = {};
            char logPath[MAX_PATH] = {};
            buildSidePath(iniPath, sizeof(iniPath), "SanAndreasWebSocket.ini");
            buildSidePath(logPath, sizeof(logPath), "SanAndreasWebSocket.log");

            int traceFlag = GetPrivateProfileIntA("Debug", "log_trace", 0, iniPath);
            Logger::init(traceFlag != 0, logPath);
        }

        Logger::trace("=== PLUGIN CONSTRUCTOR: start ===");

        // ─── ШАГИ 4-5: регистрируем обработчик initGameEvent ────────────────
        // GameThread::init() и FieldRegistry::init() НЕ вызываем здесь —
        // их внутренние std::unordered_map / std::mutex / std::queue могут
        // быть ещё не сконструированы (static init order fiasco).
        // К моменту initGameEvent все TU-static'и гарантированно готовы.

        Logger::trace("CONSTRUCTOR: registering initGameEvent handler");
        Events::initGameEvent += [] {
            Logger::trace("--- initGameEvent FIRED ---");

            // GameThread::init: подписываемся на gameProcessEvent
            Logger::trace("initGameEvent: calling GameThread::init()");
            GameThread::init();
            Logger::trace("initGameEvent: GameThread::init() OK");

            // FieldRegistry::init: заполняем карту полей
            Logger::trace("initGameEvent: calling FieldRegistry::init()");
            FieldRegistry::init();
            Logger::trace("initGameEvent: FieldRegistry::init() OK");

            // Читаем конфиг
            Logger::trace("initGameEvent: reading ini");
            char iniPath[MAX_PATH] = {};
            buildSidePath(iniPath, sizeof(iniPath), "SanAndreasWebSocket.ini");

            char host[64];
            GetPrivateProfileStringA("WebSocket", "host", "0.0.0.0",
                                     host, sizeof(host), iniPath);
            int port = GetPrivateProfileIntA("WebSocket", "port", 8765, iniPath);
            snprintf(g_bindStr, sizeof(g_bindStr), "%s:%d", host, (int)port);
            Logger::trace("initGameEvent: host=%s port=%d", host, port);

            // Создаём сервер
            Logger::trace("initGameEvent: creating WsServer endpoint");
            asio::ip::address addr;
            try { addr = asio::ip::make_address(host); }
            catch (...) {
                Logger::trace("initGameEvent: make_address failed, using any()");
                addr = asio::ip::address_v4::any();
            }

            tcp::endpoint ep(addr, static_cast<unsigned short>(port));
            Logger::trace("initGameEvent: constructing WsServer");
            g_server    = std::make_unique<WsServer>(g_ioc, ep);
            Logger::trace("initGameEvent: WsServer constructed, ok=%d",
                          g_server->ok() ? 1 : 0);

            Logger::trace("initGameEvent: creating work_guard");
            g_workGuard = std::make_unique<asio::executor_work_guard<
                asio::io_context::executor_type>>(g_ioc.get_executor());

            Logger::trace("initGameEvent: starting io_context thread");
            g_ioThread = std::thread([] {
                Logger::trace("io_context thread: running");
                g_ioc.run();
                Logger::trace("io_context thread: exited");
            });
            g_ioThread.detach();
            Logger::trace("--- initGameEvent COMPLETE ---");
        };

        Logger::trace("CONSTRUCTOR: registering drawingEvent handler");
        Events::drawingEvent += [] {
            char buf[80];
            if (!g_server) {
                gamefont::Print("WS: starting...", 10.0f, 10.0f);
            } else if (!g_server->ok()) {
                snprintf(buf, sizeof(buf), "WS FAILED: %s", g_bindStr);
                gamefont::Print(buf, 10.0f, 10.0f);
            } else {
                snprintf(buf, sizeof(buf), "WS %s | clients: %d",
                         g_bindStr, g_server->clientCount());
                gamefont::Print(buf, 10.0f, 10.0f);
            }
        };

        Logger::trace("=== PLUGIN CONSTRUCTOR: end ===");
    }
} g_plugin;
