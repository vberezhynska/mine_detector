#include "udp_socket.hpp"

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace networking {
    struct UdpSocket::Impl {
        std::string m_targetIp;
        uint16_t m_targetPort;
        int m_sockFd{-1};
        struct sockaddr_in m_destAddr{};
    };

    UdpSocket::UdpSocket(char* targetIp, uint16_t targetPort) : pImpl(std::make_unique<Impl>()) {}

    bool UdpSocket::initSocket(){
        return true;
    }

    void UdpSocket::sendCoordinates(){
        
    }
} //namespace networking