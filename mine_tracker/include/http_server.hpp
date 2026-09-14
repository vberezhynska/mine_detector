#pragma once

#include <memory>
#include <functional>
#include <cstdint>
#include <string>
#include <chrono>

namespace mine_tracker {

    struct MineAlertData {
        std::string event;
        int32_t lat;
        int32_t lon;
        int8_t gp_type;
    };

    struct StatusData {
        bool system_ok{true};
        uint32_t active_sensors{1};
    };

    class HttpServer {
        public:
            using AlertCallback = std::function<void(const MineAlertData&)>;   
            using StatusCallback = std::function<StatusData()>;
            
            // Register a callback to process incoming alerts
            bool set_alert_callback(AlertCallback callback, 
                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
            bool set_status_callback(StatusCallback callback, 
                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(100));

            bool init(uint16_t port = 8080);
            void start();
            void stop();

            HttpServer();
            ~HttpServer();

            HttpServer(const HttpServer&) = delete;
            HttpServer& operator=(const HttpServer&) = delete;
            HttpServer(HttpServer&&) noexcept;
            HttpServer& operator=(HttpServer&&) noexcept;

        private:
            struct Impl;
            std::unique_ptr<Impl> pImpl;
    };
} //namespace mine_tracker