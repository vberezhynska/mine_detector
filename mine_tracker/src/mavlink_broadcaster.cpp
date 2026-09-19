#include "mavlink_broadcaster.hpp"

#include <common/mavlink.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>

#include "confidence_engine.hpp"

namespace mine_tracker {
    struct MavlinkBroadcaster::Impl {
        int sock_fd{-1};
        struct sockaddr_in dest_addr{};
        uint8_t system_id{1};
        uint8_t component_id{MAV_COMP_ID_AUTOPILOT1};
        std::chrono::steady_clock::time_point start_time;
        std::mutex send_mutex;

        struct MineMarker {
            uint8_t sys_id;
            int32_t lat_e7;
            int32_t lon_e7;
        };
        std::mutex mines_mutex;
        // Maps group_id -> MineMarker
        std::unordered_map<uint16_t, MineMarker> active_mines;

        Impl(const std::string& broadcast_ip, uint16_t port) {
            sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_fd < 0) {
                std::cerr << "[MAVLink] Failed to create socket\n";
                return;
            }

            int broadcast_enable = 1;
            if (setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
                std::cerr << "[MAVLink] Failed to set SO_BROADCAST\n";
            }

            std::memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(port);
            inet_pton(AF_INET, broadcast_ip.c_str(), &dest_addr.sin_addr);

            start_time = std::chrono::steady_clock::now();
        }

        ~Impl() {
            if (sock_fd >= 0) {
                close(sock_fd);
            }
        }

    void send_heartbeat() {
        if (sock_fd < 0) return;

        mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

        // Rover Heartbeat (Primary vehicle, SysID = 1)
        mavlink_msg_heartbeat_pack(
            system_id,
            component_id,
            &msg,
            MAV_TYPE_GROUND_ROVER,
            MAV_AUTOPILOT_GENERIC,
            MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
            0,
            MAV_STATE_ACTIVE
        );
        uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
        {
            std::lock_guard<std::mutex> lock(send_mutex);
            sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
        }

        // Snapshot all active mines under lock
        std::vector<MineMarker> markers_to_refresh;
        {
            std::lock_guard<std::mutex> lock(mines_mutex);
            markers_to_refresh.reserve(active_mines.size());
            for (const auto& [_, marker] : active_mines) {
                markers_to_refresh.push_back(marker);
            }
        }

        auto boot_ms = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count()
        );

        // Broadcast heartbeat and position for EVERY registered mine
            for (const auto& mine : markers_to_refresh) {
            // 1. Heartbeat as a real vehicle type
            mavlink_msg_heartbeat_pack(
                mine.sys_id,
                MAV_COMP_ID_AUTOPILOT1,          // Use standard autopilot component ID
                &msg,
                MAV_TYPE_GROUND_ROVER,           // Crucial: QGC must recognize it as a trackable vehicle
                MAV_AUTOPILOT_GENERIC,
                MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                0,
                MAV_STATE_ACTIVE
            );
            len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
            }

            // 2. GPS_RAW_INT (QGC requires 3D fix to plot vehicle position on map)
            mavlink_msg_gps_raw_int_pack(
                mine.sys_id,
                MAV_COMP_ID_AUTOPILOT1,
                &msg,
                boot_ms * 1000ULL,
                GPS_FIX_TYPE_3D_FIX,             // 3 = 3D Fix
                mine.lat_e7,
                mine.lon_e7,
                0,                               // alt
                UINT16_MAX,                      // eph
                UINT16_MAX,                      // epv
                0,                               // vel
                0,                               // cog
                10,                              // satellites_visible
                0, 0, 0, 0, 0, 0
            );
            len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
            }

            // 3. GLOBAL_POSITION_INT
            mavlink_msg_global_position_int_pack(
                mine.sys_id,
                MAV_COMP_ID_AUTOPILOT1,
                &msg,
                boot_ms,
                mine.lat_e7,
                mine.lon_e7,
                0, 0, 0, 0, 0, 0
            );
            len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
            }
        
        }
    }

        void send_gps_position(int32_t lat_e7, int32_t lon_e7, int32_t alt_mm, int16_t hdg_cdeg) {
        if (sock_fd < 0) return;

            mavlink_message_t msg;
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

            auto boot_ms = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time
                ).count()
            );

            mavlink_msg_gps_raw_int_pack(
                system_id,
                component_id,
                &msg,
                boot_ms * 1000ULL,    // time_usec
                GPS_FIX_TYPE_3D_FIX,  // fix_type: 3 = 3D fix
                lat_e7,
                lon_e7,
                alt_mm,
                UINT16_MAX,           // eph (HDOP, UINT16_MAX = unknown)
                UINT16_MAX,           // epv (VDOP, UINT16_MAX = unknown)
                0,                    // vel (groundspeed cm/s)
                static_cast<uint16_t>(hdg_cdeg), // cog (course over ground, cdeg)
                10,                   // satellites_visible
                alt_mm,               // alt_ellipsoid (mm)
                0,                    // h_acc (horizontal accuracy mm, 0 = unknown)
                0,                    // v_acc (vertical accuracy mm, 0 = unknown)
                0,                    // vel_acc (speed accuracy mm/s, 0 = unknown)
                0,                    // hdg_acc (heading accuracy degE5, 0 = unknown)
                static_cast<uint16_t>(hdg_cdeg)  // yaw (heading cdeg)
            );

            uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
            }

            //(Places the vehicle pin on the map)
            mavlink_msg_global_position_int_pack(
                system_id,
                component_id,
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
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
            }
        }

        // Send STATUSTEXT notification to QGC and add mine to active_mines map
        void send_danger_zone(double radius_m, int32_t lat_e7, int32_t lon_e7, float confidence, uint16_t group_id) {
            if (sock_fd < 0){
                //TODO: update with LOG
                std::cout << "[MAVLINK] sock_fd is less then 0." << std::endl;
                return;
            } 

            uint8_t marker_sys_id = static_cast<uint8_t>(100 + (group_id % 150));
            {
                std::lock_guard<std::mutex> lock(mines_mutex);
                active_mines[group_id] = MineMarker{marker_sys_id, lat_e7, lon_e7};
            }

            //Send STATUSTEXT notification to QGC
            mavlink_message_t msg;
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
            char text[50];
            std::snprintf(text, sizeof(text), "[ALERT] Mine #%u detected! Conf: %.0f%%", group_id, confidence * 100.0f);

            mavlink_msg_statustext_pack(
                system_id,
                component_id,
                &msg,
                MAV_SEVERITY_CRITICAL,
                text,
                0,
                0
            );

            uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
                std::cout << "[MAVLINK] Added Mine #" << group_id << " to active markers (Total: " 
                        << active_mines.size() << ")" << std::endl;
            }
        }
    };

    // --- Forwarding Member Functions ---

    MavlinkBroadcaster::MavlinkBroadcaster(const std::string& broadcast_ip, uint16_t port)
        : pImpl(std::make_unique<Impl>(broadcast_ip, port)) {}

    MavlinkBroadcaster::~MavlinkBroadcaster() = default;

    MavlinkBroadcaster::MavlinkBroadcaster(MavlinkBroadcaster&&) noexcept = default;
    MavlinkBroadcaster& MavlinkBroadcaster::operator=(MavlinkBroadcaster&&) noexcept = default;

    void MavlinkBroadcaster::send_heartbeat() {
        pImpl->send_heartbeat();
    }

    void MavlinkBroadcaster::send_gps_position(int32_t lat_e7, int32_t lon_e7, int32_t alt_mm, uint16_t hdg_cdeg) {
        pImpl->send_gps_position(lat_e7, lon_e7, alt_mm, hdg_cdeg);
    }

    void MavlinkBroadcaster::send_danger_zone(double radius_m, int32_t lat_e7, int32_t lon_e7, const GroupConfidence& group_confidence) {
        pImpl->send_danger_zone(radius_m, lat_e7, lon_e7, group_confidence.confidence(), group_confidence.group_id);
    }
} //namespace mine_tracker