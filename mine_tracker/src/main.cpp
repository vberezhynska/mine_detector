#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <format>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "config/cli_args.hpp"
#include "dto/exit_codes.hpp"
#include "dto/struct_library.hpp"
#include "external/debug_macros.hpp"
#include "flags.hpp"
#include "http_server.hpp"
#include "mavlink_broadcaster.hpp"
#include "mine_alert_manager.hpp"
#include "safe_queue.hpp"
#include "udp_server.hpp"

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG(std::format("[MAIN] Shutdown signal received ({})...", signal));
        g_running.store(false);
    }
}

int main(int argc, char* argv[]) {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        mine_tracker::SafeQueue<mine_tracker::MineAlertData> alert_queue;

        // Initialize UDP Server
        constexpr uint16_t UDP_PORT = 5005;
        mine_tracker::UdpServer udp_server(UDP_PORT);
        if (!udp_server.start()) {
            LOG(std::format("[FATAL] Failed to start UDP server on port {}.", UDP_PORT));
            return static_cast<int>(ExitCode::FailedOnStartup);
        }
        DEBUG(std::format("[UDP] Server bound to port {} successfully.", UDP_PORT));

        // Mavlink setup
        const char* env_ip = std::getenv("MAVLINK_TARGET_IP");
        std::string broadcast_ip = (env_ip != nullptr) ? env_ip : "10.42.0.255";
        auto mavlink_broadcaster = std::make_shared<mine_tracker::MavlinkBroadcaster>(broadcast_ip, 14550);

        std::jthread mavlink_heartbeat_thread([mavlink_broadcaster](std::stop_token stop_token) {
            while (!stop_token.stop_requested() && g_running.load()) {
                mavlink_broadcaster->send_heartbeat();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        // UDP processing thread
        std::jthread udp_thread([&udp_server, mavlink_broadcaster](std::stop_token stop_token) {
            while (!stop_token.stop_requested() && g_running.load()) {
                auto udp_package = udp_server.receive_package();
                if (std::holds_alternative<mine_tracker::TelemetryPayload>(udp_package)) {
                    const auto& telemetry = std::get<mine_tracker::TelemetryPayload>(udp_package);

                    std::ostringstream oss;
                    oss << "[GPS DATA] Current location\n"
                        << "  ├─ Latitude:  " << std::fixed << std::setprecision(6) << telemetry.latitude << "\n"
                        << "  ├─ Longitude: " << std::fixed << std::setprecision(6) << telemetry.longitude << "\n"
                        << "  ├─ GPS Fix:   " << static_cast<int>(telemetry.gps_type) << "\n"
                        << "  └─ Timestamp: " << telemetry.timestamp << "\n";

                    DEBUG(oss.str());

                    mavlink_broadcaster->send_gps_position(
                        telemetry.latitude,
                        telemetry.longitude
                    );
                }
            }
        });

        // Alert manager worker
        double flag_radius = mine_tracker::parse_flag_radius(argc, argv);
        mine_tracker::Flags flags(flag_radius);
        mine_tracker::MineAlertManager alert_manager(alert_queue, flags, mavlink_broadcaster);
        std::jthread alert_worker(&mine_tracker::MineAlertManager::run, &alert_manager);

        // HTTP Server setup
        constexpr uint16_t HTTP_PORT = 8080;
        mine_tracker::HttpServer http_server;
        http_server.set_alert_callback([&alert_queue](const mine_tracker::MineAlertData& alert) {
            DEBUG(std::format("[HTTP MINE ALERT] Lat: {} | Lon: {}", alert.lat_int, alert.lon_int));
            alert_queue.push(alert);
        });

        std::jthread http_thread([&http_server]() {
            try {
                http_server.init(HTTP_PORT);
            } catch (const std::exception& e) {
                LOG(std::format("[FATAL] HTTP server failed on port {}: {}", HTTP_PORT, e.what()));
                g_running.store(false);
            } catch (...) {
                LOG(std::format("[FATAL] Unknown error occurred in HTTP server on port {}.", HTTP_PORT));
                g_running.store(false);
            }
        });

        LOG("System operational. Running... Press Ctrl+C to terminate.");

        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Graceful Shutdown
        LOG("[MAIN] Initiating graceful shutdown...");
        g_running.store(false);
        alert_queue.stop();

        udp_server.stop();
        http_server.stop();

        LOG("[MAIN] Stopped successfully.");
        return static_cast<int>(ExitCode::Success);

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return static_cast<int>(ExitCode::RuntimeError);
    }
}