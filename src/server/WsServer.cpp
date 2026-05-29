#include "WsServer.h"
#include "WsSession.h"

#include <boost/beast.hpp>

namespace asio  = boost::asio;
namespace beast = boost::beast;
using     tcp   = asio::ip::tcp;

WsServer::WsServer(asio::io_context& ioc, tcp::endpoint endpoint)
    : acceptor_(ioc), ioc_(ioc)
{
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) return;

    acceptor_.set_option(asio::socket_base::reuse_address(true), ec);

    acceptor_.bind(endpoint, ec);
    if (ec) return;

    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) return;

    ok_ = true;
    doAccept();
}

void WsServer::doAccept()
{
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            auto session = std::make_shared<WsSession>(std::move(socket), clientCount_);
            session->run();
        }
        if (acceptor_.is_open())
            doAccept();
    });
}
