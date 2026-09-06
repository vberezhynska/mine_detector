#pragma once

#include <string>
#include <memory>
#include "interfaces/ISocket.hpp"

namespace mine_detector { struct GpsSystemFixData; }

namespace networking {
    // Binary struct payload (packed to save airtime and eliminate string parsing overhead)
    struct __attribute__((packed)) GpsPacket {
        float latitude;
        float longitude;
        uint32_t timestamp;
    };

    class UdpSocket : ISocket {
    public:
        explicit UdpSocket(std::string server_ip, uint16_t serverPort) ;
        ~UdpSocket() override;

        UdpSocket(const UdpSocket&) = delete;
        UdpSocket& operator=(const UdpSocket&) = delete;
        UdpSocket(UdpSocket&&) noexcept;
        UdpSocket& operator=(UdpSocket&&) noexcept;

        bool init() override;
        void sendCoordinates();
        void sendTouched();

    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
        static constexpr const char* TAG = "UdpGpsSender";

        bool send_payload(const std::string& message);     
    };
} //namespace networking

