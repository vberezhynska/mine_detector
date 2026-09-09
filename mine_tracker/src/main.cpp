#include <csignal>
#include <iostream>
#include <thread>
#include <atomic>

#include "http_server.hpp"
#include "udp_server.hpp"

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[MAIN] Shutdown signal received (" << signal << ")..." << std::endl;
        g_running = false;
    }
}

int main(int argc, char* argv[]) {
    // 1. UDP SERVER
    constexpr uint16_t PORT = 5005;
    mine_tracker::UdpServer udp_server(PORT);
    
    if (!udp_server.start()){
        std::cout << "[FATAL] Failed to start UDP server on port 5005 (Port in use or socket error)." << std::endl;
        return 1;
    }

    std::cout << "[UDP] Server bound to port 5005 successfully." << std::endl;
    std::thread udp_thread([&udp_server](){
        while(g_running) {
            std::string udp_package = udp_server.receive_package();
            if (!udp_package.empty()){
                std::cout << "[UDP DATA] Received: " << udp_package << std::endl;
            }
        }
    });

    // 2. HTTP SERVER
    mine_tracker::HttpServer http_server;

    http_server.set_alert_callback([](const mine_tracker::MineAlertData& alert) {
        std::cout << "[HTTP MINE ALERT] Lat: " << alert.lat << " | Lon: " << alert.lon << std::endl;
    });

    http_server.set_status_callback([]() -> mine_tracker::StatusData {
        return { .system_ok = true, .active_sensors = 4 };
    });

    // 3. Start HTTP Server on its own background thread
    std::thread http_thread([&http_server]() {
        http_server.init(8080);
    });

    std::cout << "System operational. Press Enter to stop..." << std::endl;

    // BLOCK main thread until Enter key is pressed
    std::cin.get();

    // 4. Graceful Shutdown
    std::cout << "[MAIN] Initiating graceful shutdown..." << std::endl;
    g_running = false;
    udp_server.stop();
    if (udp_thread.joinable()) 
        udp_thread.join();

    http_server.stop();
    if (http_thread.joinable()) 
        http_thread.join();

    std::cout << "[MAIN] Stopped successfully." << std::endl;
    return 0;
}