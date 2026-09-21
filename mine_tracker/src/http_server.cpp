#include "http_server.hpp"

#include "crow.h"
#include <external/json.hpp>
#include "dto/telemetry_types.hpp"
#include "dto/struct_library.hpp"
#include "external/debug_macros.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

using json = nlohmann::json;

namespace mine_tracker {

struct HttpServer::Impl {
    crow::SimpleApp app;

    AlertCallback alert_callback{nullptr};
    StatusCallback status_callback{nullptr};

    std::timed_mutex callback_mutex;
};

HttpServer::HttpServer() : pImpl(std::make_unique<Impl>()) {}
HttpServer::~HttpServer() = default;

HttpServer::HttpServer(HttpServer&&) noexcept = default;
HttpServer& HttpServer::operator=(HttpServer&&) noexcept = default;

bool HttpServer::set_alert_callback(AlertCallback callback, std::chrono::milliseconds timeout) {
    std::unique_lock<std::timed_mutex> lock(pImpl->callback_mutex, timeout);
    
    if (!lock.owns_lock()) {
        LOG("[HttpServer] [ERROR]: Timed out waiting to lock callback_mutex.");
        return false;
    }

    pImpl->alert_callback = std::move(callback);
    return true;
}

bool HttpServer::init(uint16_t port) {
    // ==========================================
    // Route 1: POST /v1/api/alerts (Existing)
    // ==========================================
    CROW_ROUTE(pImpl->app, "/v1/api/alerts").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        try {
            // Parse JSON payload
            json payload = json::parse(req.body);

            MineAlertData data;
            data.event = payload.at("event").get<std::string>();
            data.lat_int = payload.at("lat_int").get<int32_t>();
            data.lon_int = payload.at("lon_int").get<int32_t>();
            data.gp_type = payload.at("gpType").get<int32_t>();

            DEBUG(std::format("[MINE ALERT] Event: {} | Lat: {} | Lon: {} | GpType: {}",
                              data.event,
                              data.lat_int,
                              data.lon_int,
                              to_string(static_cast<NMEA_Type>(data.gp_type))));

            // Fetch callback atomically under lock with a 100ms timeout
            AlertCallback cb_copy = nullptr;
            {
                std::unique_lock<std::timed_mutex> lock(pImpl->callback_mutex, std::chrono::milliseconds(100));
                if (lock.owns_lock()) {
                    cb_copy = pImpl->alert_callback;
                } else {
                    LOG("[HttpServer] [WARNING]: Could not acquire lock to read callback.");
                }
            } // Lock released immediately here

            // Execute callback outside the lock to prevent blocking HTTP threads
            if (cb_copy) {
                cb_copy(data);
            }

            // Return 200 OK
            json response = {{"status", "success"}};
            return crow::response(200, response.dump());

        } catch (const std::exception& e) {
            LOG(std::format("[HttpServer] Bad request: {}", e.what()));
            json err_response = {{"status", "error"}, {"message", "Invalid JSON or missing fields"}};
            return crow::response(400, err_response.dump());
        }
    });


    // Start server (blocking call)
    LOG(std::format("Mine Alert Server running on port {}...", port));
    pImpl->app.signal_clear(); // Disables Crow's internal SIGINT/SIGTERM handlers
    pImpl->app.port(port).multithreaded().run();

    return true;
}

void HttpServer::stop() {
    pImpl->app.stop();
}

} // namespace mine_tracker