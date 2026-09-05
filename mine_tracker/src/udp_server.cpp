#include "udp_server.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <netinet/in.h>

namespace mine_tracker {
    struct UdpServer::Impl{
        uint16_t port;
        int socket_fd{-1};
        bool is_running{false};

        explicit Impl(uint16_t port) : port(port){}
    };

    UdpServer::UdpServer(uint16_t port) 
        : pImpl(std::make_unique<Impl>(port)){}

    UdpServer::~UdpServer() {
        stop();
    }

    void UdpServer::stop(){
        if (pImpl->socket_fd >= 0){
            ::close(pImpl->socket_fd);
            pImpl->socket_fd = -1;
        }

        pImpl->is_running = false;
    }

    bool UdpServer::start(){
        pImpl->socket_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (pImpl->socket_fd < 0) {
            return false;
        }

        int opt = 1;
        ::setsockopt(pImpl->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(pImpl->port);

        if (::bind(pImpl->socket_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            ::close(pImpl->socket_fd);
            pImpl->socket_fd = -1;
            return false;
        }

        pImpl->is_running = true;
        return true;
    }

    std::string UdpServer::receive_package(){
        if (!pImpl->is_running) return "";

        char buffer[1024] = {0}; //TODO: increase when work with GPS data
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        // Blocking wait for incoming data from ESP32
        ssize_t bytes_received = ::recvfrom(pImpl->socket_fd, buffer, sizeof(buffer) - 1, 0,
                                        reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);

            if (bytes_received > 0) {
                return std::string(buffer, bytes_received);
            }

        return "";
    }
} //namespace mine_tracker