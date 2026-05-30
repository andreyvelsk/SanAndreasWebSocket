#pragma once

#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>

class WsSession : public std::enable_shared_from_this<WsSession> {
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::beast::flat_buffer                                      buf_;
    std::deque<std::string>                                        writeQueue_;
    bool                                                           writing_ = false;
    std::atomic<int>&                                              clientCount_;

    // ── subscribe state ──────────────────────────────────────────────────────
    std::set<std::string>              subscribedFields_;
    std::map<std::string, nlohmann::json> previousValues_;
    std::chrono::milliseconds          subscribeInterval_{500};
    boost::asio::steady_timer          subscribeTimer_;
    bool                               timerRunning_ = false;

public:
    WsSession(boost::asio::ip::tcp::socket socket, std::atomic<int>& clientCount);

    void run();
    void send(std::string msg);
    auto executor() { return ws_.get_executor(); }

private:
    void doRead();
    void doWrite();
    void handleMessage(const std::string& msg);

    // JSON-RPC 2.0 helpers
    static nlohmann::json makeResult(const nlohmann::json& id, nlohmann::json result);
    static nlohmann::json makeError(const nlohmann::json& id, int code, std::string message);

    // Subscribe push loop
    void startSubscribeTimer();
    void cancelSubscribeTimer();
};
