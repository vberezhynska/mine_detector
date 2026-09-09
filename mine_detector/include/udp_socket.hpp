#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

#include "interfaces/ISocket.hpp"
#include "external/telemetry_types.hpp"


namespace mine_detector { struct GpsSystemFixData; }

namespace networking {
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

    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
        static constexpr const char* TAG = "UdpGpsSender";

        bool send_payload(const void* data, std::size_t size);     
    };
} //namespace networking