#include "mavlink_broadcaster.hpp"

#include <common/mavlink.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>

namespace mine_tracker {
    struct MavlinkBroadcaster::Impl {
        int sock_fd_{-1};
        struct sockaddr_in dest_addr{};
        uint8_t system_id_{1};
        uint8_t component_id_{MAV_COMP_ID_AUTOPILOT1};
        std::chrono::steady_clock::time_point start_time;
        std::mutex send_mutex;

        Impl(const std::string& broadcast_ip, uint16_t port) {
            sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_fd_ < 0) {
                std::cerr << "[MAVLink] Failed to create socket\n";
                return;
            }

            int broadcast_enable = 1;
            if (setsockopt(sock_fd_, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
                std::cerr << "[MAVLink] Failed to set SO_BROADCAST\n";
            }

            std::memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(port);
            inet_pton(AF_INET, broadcast_ip.c_str(), &dest_addr.sin_addr);

            start_time = std::chrono::steady_clock::now();
        }

        ~Impl() {
            if (sock_fd_ >= 0) {
                close(sock_fd_);
            }
        }

        void send_heartbeat() {
            if (sock_fd_ < 0) return;

            mavlink_message_t msg;
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

            mavlink_msg_heartbeat_pack(
                system_id_,
                component_id_,
                &msg,
                MAV_TYPE_GROUND_ROVER,
                MAV_AUTOPILOT_GENERIC,
                MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                0,
                MAV_STATE_ACTIVE
            );

            uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
            std::lock_guard<std::mutex> lock(send_mutex);
            sendto(sock_fd_, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
        }

        void send_gps_position(int32_t lat_e7, int32_t lon_e7, int32_t alt_mm, int16_t hdg_cdeg) {
        if (sock_fd_ < 0) return;

            mavlink_message_t msg;
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

            auto boot_ms = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time
                ).count()
            );

            mavlink_msg_gps_raw_int_pack(
                system_id_,
                component_id_,
                &msg,
                boot_ms * 1000ULL, // time_usec
                GPS_FIX_TYPE_3D_FIX, // fix_type: 3 = 3D fix
                lat_e7,
                lon_e7,
                alt_mm,
                UINT16_MAX,        // eph (HDOP, unknown)
                UINT16_MAX,        // epv (VDOP, unknown)
                0,                 // vel (groundspeed cm/s)
                hdg_cdeg,          // cog (course over ground)
                10                 // satellites_visible (e.g. 10 sats)
            );

            uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);

            std::lock_guard<std::mutex> lock(send_mutex);
            sendto(sock_fd_, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));

            //(Places the vehicle pin on the map)
            mavlink_msg_global_position_int_pack(
                system_id_,
                component_id_,
                &msg,
                boot_ms,
                lat_e7,
                lon_e7,
                alt_mm,
                alt_mm,
                0, 0, 0,
                hdg_cdeg
            );

            len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex_);
                sendto(sock_fd_, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr_), sizeof(dest_addr_));
            }
        }
    };

    // --- Forwarding Member Functions ---

    MavlinkBroadcaster::MavlinkBroadcaster(const std::string& broadcast_ip, uint16_t port)
        : pImpl(std::make_unique<Impl>(broadcast_ip, port)) {}

    // Must be defined in the .cpp file so unique_ptr knows the complete type of Impl
    MavlinkBroadcaster::~MavlinkBroadcaster() = default;

    MavlinkBroadcaster::MavlinkBroadcaster(MavlinkBroadcaster&&) noexcept = default;
    MavlinkBroadcaster& MavlinkBroadcaster::operator=(MavlinkBroadcaster&&) noexcept = default;

    void MavlinkBroadcaster::send_heartbeat() {
        pImpl->send_heartbeat();
    }

    void MavlinkBroadcaster::send_gps_position(int32_t lat_e7, int32_t lon_e7, int32_t alt_mm, uint16_t hdg_cdeg) {
        pImpl->send_gps_position(lat_e7, lon_e7, alt_mm, hdg_cdeg);
    }
} //namespace mine_tracker