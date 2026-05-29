#include "server/WsServer.h"

#include "plugin.h"
#include "extensions/FontPrint.h"

#include <boost/asio.hpp>
#include <memory>
#include <thread>

namespace asio = boost::asio;
using     tcp  = asio::ip::tcp;

// ---------- чтение SanAndreasWebSocket.ini (Windows API из /FI windows.h) ---
static std::string getIniPath() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    char* slash = strrchr(buf, '\\');
    if (slash) strcpy_s(slash + 1, MAX_PATH - (DWORD)(slash + 1 - buf),
                        "SanAndreasWebSocket.ini");
    else       strcpy_s(buf, sizeof(buf), "SanAndreasWebSocket.ini");
    return buf;
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
        Events::initGameEvent += [] {
            // --- читаем конфиг ---
            std::string ini = getIniPath();
            char host[64];
            GetPrivateProfileStringA("WebSocket", "host", "0.0.0.0",
                                     host, sizeof(host), ini.c_str());
            int port = GetPrivateProfileIntA("WebSocket", "port", 8765,
                                              ini.c_str());
            snprintf(g_bindStr, sizeof(g_bindStr), "%s:%d", host, (int)port);

            // --- создаём сервер ---
            asio::ip::address addr;
            try { addr = asio::ip::make_address(host); }
            catch (...) { addr = asio::ip::address_v4::any(); }

            tcp::endpoint ep(addr, static_cast<unsigned short>(port));
            g_server    = std::make_unique<WsServer>(g_ioc, ep);
            g_workGuard = std::make_unique<asio::executor_work_guard<
                asio::io_context::executor_type>>(g_ioc.get_executor());
            g_ioThread  = std::thread([] { g_ioc.run(); });
            g_ioThread.detach();
        };

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
    }
} g_plugin;
 