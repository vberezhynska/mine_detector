#include <iostream>
#include "udp_server.hpp"

// test from PS
//echo TOUCH_EVENT_FROM_PC | ncat -u 10.42.0.1 5005

int main(int argc, char* argv[]) {
    constexpr uint16_t PORT = 5005;
    mine_tracker::UdpServer server(PORT);

    if (!server.start())
    {
        std::cout << "Can not start server." << std::endl;
        return 1;
    }
        
    std::cout << "=========================================\n";
    std::cout << "  Mine Tracker - Waiting for Touch Events \n";
    std::cout << "  Listening on UDP port " << PORT << "...\n";
    std::cout << "=========================================\n\n";

    while(true){
        std::string message = server.receive_package();
        if (!message.empty()) {
            std::cout << "[EVENT] Touch detected! Payload: " << message << std::endl;
        }
    }

    return 0;
}