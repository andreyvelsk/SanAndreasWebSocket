#pragma once

#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

#include <atomic>
#include <deque>
#include <memory>
#include <string>

class WsSession : public std::enable_shared_from_this<WsSession> {
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::beast::flat_buffer                                      buf_;
    std::deque<std::string>                                        writeQueue_;
    bool                                                           writing_ = false;
    std::atomic<int>&                                              clientCount_;

public:
    WsSession(boost::asio::ip::tcp::socket socket, std::atomic<int>& clientCount);

    void run();
    void send(std::string msg);

private:
    void doRead();
    void doWrite();
    void handleMessage(const std::string& msg);
};
