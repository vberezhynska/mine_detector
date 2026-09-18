#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace mine_tracker {
    class MavlinkBroadcaster {
    public:
        explicit MavlinkBroadcaster(const std::string& broadcast_ip = "192.168.4.255", uint16_t port = 14550);
        ~MavlinkBroadcaster();

        MavlinkBroadcaster(MavlinkBroadcaster&&) noexcept;
        MavlinkBroadcaster& operator=(MavlinkBroadcaster&&) noexcept;
        MavlinkBroadcaster(const MavlinkBroadcaster&) = delete;
        MavlinkBroadcaster& operator=(const MavlinkBroadcaster&) = delete;

        void send_heartbeat();
        void send_gps_position(int32_t lat_e7, int32_t lon_e7, int32_t alt_mm = 0, uint16_t hdg_cdeg = 0);
    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
    };
} //namespace mine_tracker
