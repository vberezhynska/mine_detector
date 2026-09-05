#pragma once

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
        explicit UdpSocket(char* targetIp, uint16_t targetPort);
        ~UdpSocket() override;

        UdpSocket(const UdpSocket&) = delete;
        UdpSocket& operator=(const UdpSocket&) = delete;
        UdpSocket(UdpSocket&&) noexcept;
        UdpSocket& operator=(UdpSocket&&) noexcept;

        bool initSocket() override;
        void sendCoordinates();

    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
        
        static constexpr const char* TAG = "UdpGpsSender";
    };
} //namespace networking

