#include <csignal>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include "dto/struct_library.hpp"
#include "mine_alert_manager.hpp"
#include "db_manager.hpp"
#include "http_server.hpp"
#include "udp_server.hpp"
#include "flags.hpp"
#include "safe_queue.hpp"
#include "mavlink_broadcaster.hpp"

inline constexpr double FLAG_RADIUS = 30.0; //30m

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[MAIN] Shutdown signal received (" << signal << ")..." << std::endl;
        g_running.store(false);
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    mine_tracker::SafeQueue<mine_tracker::MineAlertData> alert_queue;
    
    // SQLite DB
    #ifndef DB_PATH
    #define DB_PATH "mines.db"
    #endif

    const std::string db_file = DB_PATH;
    auto db = std::make_shared<data::DbManager>(db_file);
    std::cout << "[DB] Initialized database at: " << db_file << "\n";

    // MavLINK
    auto mavlink_broadcaster = std::make_shared<mine_tracker::MavlinkBroadcaster>("192.168.1.222", 14550);

    std::thread mavlink_heartbeat_thread([mavlink_broadcaster]() {
        while (g_running.load()) {
            mavlink_broadcaster->send_heartbeat();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    // UDP SERVER
    constexpr uint16_t PORT = 5005;
    mine_tracker::UdpServer udp_server(PORT);
    
    if (!udp_server.start()) {
        std::cout << "[FATAL] Failed to start UDP server on port 5005." << std::endl;
        return 1;
    }

    std::cout << "[UDP] Server bound to port 5005 successfully." << std::endl;
    std::thread udp_thread([&udp_server, mavlink_broadcaster]() {
        while (g_running.load()) {
            auto udp_package = udp_server.receive_package();
            if (std::holds_alternative<mine_tracker::TelemetryPayload>(udp_package)) {
                const auto& telemetry = std::get<mine_tracker::TelemetryPayload>(udp_package);
                
                std::cout << "[UDP DATA] Mine Alert Received!\n"
                          << "  ├─ Latitude:  " << telemetry.latitude << "\n"
                          << "  ├─ Longitude: " << telemetry.longitude << "\n"
                          << "  ├─ GPS Fix:   " << static_cast<int>(telemetry.gps_type) << "\n"
                          << "  └─ Timestamp: " << telemetry.timestamp << std::endl;
                
                mavlink_broadcaster->send_gps_position(
                    telemetry.latitude, 
                    telemetry.longitude
                );
            }
        }
    });

    //TODO: move radius to configuration
    // Alert manager worker
    mine_tracker::Flags flags(FLAG_RADIUS);
    mine_tracker::MineAlertManager alert_manager(alert_queue, flags);
    std::jthread alert_worker(&mine_tracker::MineAlertManager::run, &alert_manager);

    // HTTP SERVER
    mine_tracker::HttpServer http_server;
    
    http_server.set_alert_callback([&alert_queue](const mine_tracker::MineAlertData& alert) {
        std::cout << "[HTTP MINE ALERT] Lat: " << alert.lat_int << " | Lon: " << alert.lon_int << std::endl;
        alert_queue.push(alert);
    });

    http_server.set_status_callback([]() -> mine_tracker::StatusData { //TODO: remove for production. Just for testing
        return { .system_ok = true, .active_sensors = 4 };
    });

    // Start HTTP Server in thread
    std::thread http_thread([&http_server]() {
        http_server.init(8080);
    });

    std::cout << "System operational. Running... Press Ctrl+C to terminate." << std::endl;

    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Graceful Shutdown
    std::cout << "[MAIN] Initiating graceful shutdown..." << std::endl;
    alert_queue.stop();

    udp_server.stop();
    if (udp_thread.joinable()) {
        udp_thread.join();
    }

    http_server.stop();
    if (http_thread.joinable()) {
        http_thread.join();
    }

    if (mavlink_heartbeat_thread.joinable()) {
        mavlink_heartbeat_thread.join();
    }

    // alert_worker automatically joins here via std::jthread destructor

    std::cout << "[MAIN] Stopped successfully." << std::endl;
    return 0;
}