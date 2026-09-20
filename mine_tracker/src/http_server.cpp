#include "http_server.hpp"

#include "crow.h"
#include <external/json.hpp>
#include "dto/telemetry_types.hpp"
#include "dto/struct_library.hpp"

#include <iostream>
#include <mutex>
#include <utility>
#include <memory>
#include <functional>
#include <cstdint>
#include <string>
#include <chrono>

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
        std::cerr << "[HttpServer] Error: Timed out waiting to lock callback_mutex." << std::endl;
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
            // 1. Parse JSON payload
            json payload = json::parse(req.body);

            MineAlertData data;
            data.event = payload.at("event").get<std::string>();
            data.lat_int = payload.at("lat_int").get<int32_t>();
            data.lon_int = payload.at("lon_int").get<int32_t>();
            data.gp_type = payload.at("gpType").get<int32_t>();

            std::cout << "[MINE ALERT] Event: " << data.event 
                      << " | Lat: "     << data.lat_int 
                      << " | Lon: "     << data.lon_int << std::endl
                      << " | GpType: "  << to_string(static_cast<NMEA_Type>(data.gp_type)) << std::endl;

            // 2. Fetch callback atomically under lock with a 100ms timeout
            AlertCallback cb_copy = nullptr;
            {
                std::unique_lock<std::timed_mutex> lock(pImpl->callback_mutex, std::chrono::milliseconds(100));
                if (lock.owns_lock()) {
                    cb_copy = pImpl->alert_callback;
                } else {
                    std::cerr << "[HttpServer] Warning: Could not acquire lock to read callback." << std::endl;
                }
            } // Lock released immediately here

            // 3. Execute callback outside the lock to prevent blocking HTTP threads
            if (cb_copy) {
                cb_copy(data);
            }

            // 4. Return 200 OK
            json response = {{"status", "success"}};
            return crow::response(200, response.dump());

        } catch (const std::exception& e) {
            std::cerr << "[HttpServer] Bad request: " << e.what() << std::endl;
            json err_response = {{"status", "error"}, {"message", "Invalid JSON or missing fields"}};
            return crow::response(400, err_response.dump());
        }
    });

    // ==========================================
    // Route 2: GET /v1/api/status (New GET)
    // ==========================================
    CROW_ROUTE(pImpl->app, "/v1/api/status").methods(crow::HTTPMethod::GET)
    ([this]() {
        StatusCallback cb_copy = nullptr;
        {
            std::unique_lock<std::timed_mutex> lock(pImpl->callback_mutex, std::chrono::milliseconds(100));
            if (lock.owns_lock()) {
                cb_copy = pImpl->status_callback;
            }
        }

        if (cb_copy) {
            StatusData status = cb_copy(); // Query main pipeline state
            
            json res = {
                {"status", status.system_ok ? "ok" : "degraded"},
                {"sensors", status.active_sensors}
            };
            return crow::response(200, res.dump());
        }

        return crow::response(503, "{\"error\":\"Status handler not ready\"}");
    });

    // Start server (blocking call)
    std::cout << "Mine Alert Server running on port " << port << "..." << std::endl;
    pImpl->app.signal_clear(); // Disables Crow's internal SIGINT/SIGTERM handlers
    pImpl->app.port(port).multithreaded().run();

    return true;
}

void HttpServer::stop() {
    pImpl->app.stop();
}

} // namespace mine_tracker