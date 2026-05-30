#include "WsSession.h"

#include "../GameThread.h"
#include "../Logger.h"
#include "../protocol/FieldRegistry.h"

#include <nlohmann/json.hpp>
#include <windows.h>   // GetTickCount

namespace asio  = boost::asio;
namespace beast = boost::beast;

// ── ctor ─────────────────────────────────────────────────────────────────────

WsSession::WsSession(asio::ip::tcp::socket socket, std::atomic<int>& clientCount)
    : ws_(std::move(socket))
    , clientCount_(clientCount)
    , subscribeTimer_(ws_.get_executor())
{}

// ── public ───────────────────────────────────────────────────────────────────

void WsSession::run()
{
    ws_.set_option(beast::websocket::stream_base::timeout::suggested(
        beast::role_type::server));

    ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
        if (ec) {
            Logger::trace("WsSession: accept error: %s", ec.message().c_str());
            return;
        }
        ++self->clientCount_;
        Logger::trace("WsSession: client connected (total=%d)", (int)self->clientCount_);
        self->ws_.text(true);
        self->doRead();
    });
}

void WsSession::send(std::string msg)
{
    Logger::trace("WsSession: send: %s", msg.c_str());
    writeQueue_.push_back(std::move(msg));
    if (!writing_)
        doWrite();
}

// ── private: io ──────────────────────────────────────────────────────────────

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
            Logger::trace("WsSession: read closed/error: %s", ec.message().c_str());
            self->cancelSubscribeTimer();
            --self->clientCount_;
            return;
        }
        std::string msg = beast::buffers_to_string(self->buf_.data());
        self->buf_.consume(self->buf_.size());
        Logger::trace("WsSession: recv: %s", msg.c_str());
        self->handleMessage(msg);
        self->doRead();
    });
}

// ── JSON-RPC 2.0 helpers ─────────────────────────────────────────────────────

nlohmann::json WsSession::makeResult(const nlohmann::json& id, nlohmann::json result)
{
    return {{"jsonrpc", "2.0"}, {"result", std::move(result)}, {"id", id}};
}

nlohmann::json WsSession::makeError(const nlohmann::json& id, int code, std::string message)
{
    return {{"jsonrpc", "2.0"},
            {"error",   {{"code", code}, {"message", std::move(message)}}},
            {"id",      id}};
}

// ── subscribe timer ──────────────────────────────────────────────────────────

// ── sendSnapshot: читает поля прямо сейчас и отправляет все (без diff) ─────────

void WsSession::sendSnapshot(std::set<std::string> fields)
{
    if (fields.empty()) return;
    auto self = shared_from_this();
    GameThread::post([self, fields = std::move(fields)]() {
        Logger::trace("WsSession: sendSnapshot — reading %d field(s) in game-thread",
                      (int)fields.size());
        nlohmann::json current = nlohmann::json::object();
        for (const auto& f : fields)
            current[f] = FieldRegistry::get(f);

        asio::post(self->executor(),
            [self, current = std::move(current)]() mutable {
                // Update previousValues_ so the timer won't re-send the same data
                for (auto& [k, v] : current.items())
                    self->previousValues_[k] = v;

                nlohmann::json notif = {
                    {"jsonrpc", "2.0"},
                    {"method",  "data"},
                    {"params",  {
                        {"ts",     static_cast<uint64_t>(GetTickCount())},
                        {"fields", std::move(current)}
                    }}
                };
                self->send(notif.dump());
            });
    });
}

void WsSession::startSubscribeTimer()
{
    if (subscribedFields_.empty()) {
        timerRunning_ = false;
        return;
    }
    timerRunning_ = true;
    subscribeTimer_.expires_after(subscribeInterval_);
    subscribeTimer_.async_wait([self = shared_from_this()](boost::system::error_code ec) {
        if (ec) { self->timerRunning_ = false; return; }

        // Capture current subscribed fields (copy, safe on io-thread)
        auto fields = self->subscribedFields_;
        if (fields.empty()) { self->timerRunning_ = false; return; }

        GameThread::post([self, fields = std::move(fields)]() {
            // ── game-thread: read fields ──────────────────────────────────
            Logger::trace("WsSession: subscribe timer — reading %d field(s) in game-thread",
                          (int)fields.size());
            nlohmann::json current = nlohmann::json::object();
            for (const auto& f : fields)
                current[f] = FieldRegistry::get(f);

            // Post result back to io-thread
            asio::post(self->executor(),
                [self, current = std::move(current)]() mutable {
                    // ── io-thread: diff & send ────────────────────────────
                    nlohmann::json diff = nlohmann::json::object();
                    for (auto& [k, v] : current.items()) {
                        auto it = self->previousValues_.find(k);
                        if (it == self->previousValues_.end() || it->second != v) {
                            diff[k]                  = v;
                            self->previousValues_[k] = v;
                        }
                    }
                    if (!diff.empty()) {
                        nlohmann::json notif = {
                            {"jsonrpc", "2.0"},
                            {"method",  "data"},
                            {"params",  {
                                {"ts",     static_cast<uint64_t>(GetTickCount())},
                                {"fields", std::move(diff)}
                            }}
                        };
                        self->send(notif.dump());
                    }
                    // Restart timer if still subscribed
                    if (!self->subscribedFields_.empty())
                        self->startSubscribeTimer();
                    else
                        self->timerRunning_ = false;
                });
        });
    });
}

void WsSession::cancelSubscribeTimer()
{
    subscribeTimer_.cancel();
    timerRunning_     = false;
    subscribedFields_.clear();
}

// ── message handler ──────────────────────────────────────────────────────────

void WsSession::handleMessage(const std::string& msg)
{
    // ── parse ─────────────────────────────────────────────────────────────────
    nlohmann::json j;
    try { j = nlohmann::json::parse(msg); }
    catch (...) {
        send(makeError(nullptr, -32700, "Parse error").dump());
        return;
    }

    // ── validate JSON-RPC 2.0 ────────────────────────────────────────────────
    if (!j.is_object()
        || !j.contains("method") || !j["method"].is_string()
        || j.value("jsonrpc", "") != "2.0")
    {
        nlohmann::json id = j.contains("id") ? j["id"] : nlohmann::json(nullptr);
        send(makeError(id, -32600, "Invalid Request").dump());
        return;
    }

    const bool     isNotification = !j.contains("id");
    nlohmann::json id    = isNotification ? nlohmann::json(nullptr) : j["id"];
    const auto     method = j["method"].get<std::string>();
    const auto     params = j.value("params", nlohmann::json::object());

    // ── ping ─────────────────────────────────────────────────────────────────
    if (method == "ping") {
        if (!isNotification) send(makeResult(id, nullptr).dump());
        return;
    }

    // ── query ─────────────────────────────────────────────────────────────────
    if (method == "query") {
        if (!params.contains("fields") || !params["fields"].is_array()) {
            send(makeError(id, -32602, "Invalid params: 'fields' array required").dump());
            return;
        }
        auto fields = params["fields"].get<std::vector<std::string>>();

        // Validate field names immediately (io-thread)
        for (const auto& f : fields) {
            if (!FieldRegistry::has(f)) {
                send(makeError(id, -32002, "Unknown field: " + f).dump());
                return;
            }
        }

        auto self = shared_from_this();
        GameThread::post([self, id, fields]() {
            Logger::trace("WsSession: query — reading %d field(s) in game-thread",
                          (int)fields.size());
            nlohmann::json fieldResult = nlohmann::json::object();
            for (const auto& f : fields)
                fieldResult[f] = FieldRegistry::get(f);

            auto resp = makeResult(id, {
                {"ts",     static_cast<uint64_t>(GetTickCount())},
                {"fields", std::move(fieldResult)}
            });
            asio::post(self->executor(), [self, s = resp.dump()]() {
                self->send(s);
            });
        });
        return;
    }

    // ── subscribe ─────────────────────────────────────────────────────────────
    if (method == "subscribe") {
        if (!params.contains("fields") || !params["fields"].is_array()) {
            if (!isNotification)
                send(makeError(id, -32602, "Invalid params: 'fields' array required").dump());
            return;
        }
        auto fields = params["fields"].get<std::vector<std::string>>();

        // Validate field names
        for (const auto& f : fields) {
            if (!FieldRegistry::has(f)) {
                if (!isNotification)
                    send(makeError(id, -32002, "Unknown field: " + f).dump());
                return;
            }
        }

        // Optional interval (default 500, min 50 ms)
        if (params.contains("interval") && params["interval"].is_number_integer()) {
            int ms = params["interval"].get<int>();
            if (ms < 50) ms = 50;
            subscribeInterval_ = std::chrono::milliseconds(ms);
        }

        for (const auto& f : fields)
            subscribedFields_.insert(f);

        // Send confirmation before async snapshot
        if (!isNotification) {
            nlohmann::json subList = nlohmann::json::array();
            for (const auto& f : subscribedFields_) subList.push_back(f);
            send(makeResult(id, {{"subscribed", std::move(subList)},
                                  {"interval", subscribeInterval_.count()}}).dump());
        }

        // Immediate snapshot of all subscribed fields
        sendSnapshot(subscribedFields_);

        // Start interval timer if not already running
        if (!timerRunning_)
            startSubscribeTimer();
        return;
    }

    // ── unsubscribe ───────────────────────────────────────────────────────────
    if (method == "unsubscribe") {
        if (!params.contains("fields") || !params["fields"].is_array()) {
            if (!isNotification)
                send(makeError(id, -32602, "Invalid params: 'fields' array required").dump());
            return;
        }
        auto fields = params["fields"].get<std::vector<std::string>>();
        for (const auto& f : fields) {
            subscribedFields_.erase(f);
            previousValues_.erase(f);
        }
        if (!isNotification) send(makeResult(id, nullptr).dump());
        return;
    }

    // ── unsubscribe_all ───────────────────────────────────────────────────────
    if (method == "unsubscribe_all") {
        cancelSubscribeTimer();
        previousValues_.clear();
        if (!isNotification) send(makeResult(id, nullptr).dump());
        return;
    }

    // ── unknown method ────────────────────────────────────────────────────────
    if (!isNotification)
        send(makeError(id, -32601, "Method not found: " + method).dump());
}

