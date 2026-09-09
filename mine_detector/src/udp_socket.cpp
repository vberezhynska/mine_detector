#include "udp_socket.hpp"

#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

namespace networking {
    struct UdpSocket::Impl {
        std::string server_ip;
        uint16_t server_port;
        int socket_fd{-1};
        struct sockaddr_in server_addr{};

        Impl(std::string ip, uint16_t port) 
        : server_ip(std::move(ip)), server_port(port) {}
        };

    UdpSocket::UdpSocket(std::string server_ip, uint16_t serverPort) 
        : pImpl(std::make_unique<Impl>(move(server_ip), serverPort)) {}

    UdpSocket::~UdpSocket(){
        if (pImpl->socket_fd >= 0) {
            ::close(pImpl->socket_fd);
        }
    }

    bool UdpSocket::init(){
        pImpl->socket_fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (pImpl->socket_fd < 0){
            ESP_LOGE(TAG, "Failed to create UDP socket.");
            return false;
        }

        std::memset(&pImpl->server_addr, 0, sizeof(pImpl->server_addr));
        pImpl->server_addr.sin_family = AF_INET;
        pImpl->server_addr.sin_port = htons(pImpl->server_port);

        if (::inet_pton(AF_INET, pImpl->server_ip.c_str(), &pImpl->server_addr.sin_addr) <= 0) {
            ESP_LOGE(TAG, "Invalid IP address");
            ::close(pImpl->socket_fd);
            pImpl->socket_fd = -1;
            return false;
        }

        return true;
    }

    void UdpSocket::sendCoordinates(){
        
    }

    void UdpSocket::sendTouched(){
        std::string mesage = "[UDP Socket] Mine detected!";
        auto isSent = send_payload(mesage);

        if (!isSent)
            ESP_LOGE(TAG, "Send message failed");
    }

    bool UdpSocket::send_payload(const std::string& message) {
        if (pImpl->socket_fd < 0) return false;

        ssize_t bytes_sent = ::sendto(
            pImpl->socket_fd,
            message.c_str(),
            message.length(),
            0,
            reinterpret_cast<struct sockaddr*>(&pImpl->server_addr),
            sizeof(pImpl->server_addr)
        );

        return (bytes_sent == static_cast<ssize_t>(message.length()));
    }
} //namespace networking