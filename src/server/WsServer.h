#pragma once

#include <boost/asio.hpp>
#include <atomic>
#include <memory>

class WsSession;

class WsServer {
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context&       ioc_;
    std::atomic<int>               clientCount_{0};
    bool                           ok_ = false;

public:
    WsServer(boost::asio::io_context& ioc,
             boost::asio::ip::tcp::endpoint endpoint);

    bool ok()          const { return ok_; }
    int  clientCount() const { return clientCount_.load(); }

private:
    void doAccept();

    friend class WsSession;
};
