#include "server/WsServer.h"

#include "plugin.h"
#include "extensions/FontPrint.h"

#include <boost/asio.hpp>
#include <memory>
#include <thread>

namespace asio = boost::asio;
using     tcp  = asio::ip::tcp;

static asio::io_context                                              g_ioc;
static std::unique_ptr<WsServer>                                     g_server;
static std::thread                                                   g_ioThread;
static std::unique_ptr<asio::executor_work_guard<
    asio::io_context::executor_type>>                                g_workGuard;

using namespace plugin;

static struct SanAndreasWebSocket {
    SanAndreasWebSocket() {
        Events::initGameEvent += [] {
            tcp::endpoint ep(asio::ip::make_address("127.0.0.1"), 8765);
            g_server    = std::make_unique<WsServer>(g_ioc, ep);
            g_workGuard = std::make_unique<asio::executor_work_guard<
                asio::io_context::executor_type>>(g_ioc.get_executor());
            g_ioThread  = std::thread([] { g_ioc.run(); });
            g_ioThread.detach();
        };

        Events::drawingEvent += [] {
            char buf[64];
            if (!g_server) {
                gamefont::Print("WS: starting...", 10.0f, 10.0f);
            } else if (!g_server->ok()) {
                gamefont::Print("WS: FAILED to bind :8765", 10.0f, 10.0f);
            } else {
                snprintf(buf, sizeof(buf), "WS :8765 | clients: %d",
                         g_server->clientCount());
                gamefont::Print(buf, 10.0f, 10.0f);
            }
        };
    }
} g_plugin;
