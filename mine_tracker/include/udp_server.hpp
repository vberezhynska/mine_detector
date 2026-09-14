#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include "dto/telemetry_types.hpp"

namespace mine_tracker {   
    class UdpServer {
        public:
            explicit UdpServer(uint16_t port);
            ~UdpServer();

            UdpServer(const UdpServer&) = delete;
            UdpServer& operator=(const UdpServer&) = delete;
            UdpServer(UdpServer&&) noexcept = default;
            UdpServer& operator=(UdpServer&&) noexcept = default;

            bool start();
            void stop();

            //Blocking call to receive a single package
            UdpPacket receive_package();

            private:
                struct Impl;
                std::unique_ptr<Impl> pImpl;
    };

} //namespace mine_tracker