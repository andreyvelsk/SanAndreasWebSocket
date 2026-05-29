#include "WsSession.h"

#include <nlohmann/json.hpp>

namespace asio  = boost::asio;
namespace beast = boost::beast;

WsSession::WsSession(asio::ip::tcp::socket socket, std::atomic<int>& clientCount)
    : ws_(std::move(socket)), clientCount_(clientCount)
{}

void WsSession::run()
{
    // Recommended timeouts for server role
    ws_.set_option(beast::websocket::stream_base::timeout::suggested(
        beast::role_type::server));

    ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
        if (ec) return;
        ++self->clientCount_;
        self->ws_.text(true);
        self->doRead();
    });
}

void WsSession::send(std::string msg)
{
    writeQueue_.push_back(std::move(msg));
    if (!writing_)
        doWrite();
}

void WsSession::doWrite()
{
    if (writeQueue_.empty()) {
        writing_ = false;
        return;
    }
    writing_ = true;
    ws_.async_write(asio::buffer(writeQueue_.front()),
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (!self->writeQueue_.empty())
                self->writeQueue_.pop_front();
            if (ec) {
                self->writing_ = false;
                self->writeQueue_.clear();
                return;
            }
            self->doWrite();
        });
}

void WsSession::doRead()
{
    ws_.async_read(buf_, [self = shared_from_this()](beast::error_code ec, std::size_t) {
        if (ec) {
            --self->clientCount_;
            return;
        }
        std::string msg = beast::buffers_to_string(self->buf_.data());
        self->buf_.consume(self->buf_.size());
        self->handleMessage(msg);
        self->doRead();
    });
}

void WsSession::handleMessage(const std::string& msg)
{
    try {
        auto j = nlohmann::json::parse(msg);
        if (j.value("type", "") == "ping") {
            send(nlohmann::json{{"type", "pong"}}.dump());
        }
    } catch (...) {
        // ignore malformed JSON
    }
}
