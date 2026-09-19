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

#include "confidence_engine.hpp"

namespace mine_tracker {
    struct MavlinkBroadcaster::Impl {
        int sock_fd{-1};
        struct sockaddr_in dest_addr{};
        uint8_t system_id{1};
        uint8_t component_id{MAV_COMP_ID_AUTOPILOT1};
        std::chrono::steady_clock::time_point start_time;
        std::mutex send_mutex;

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
            std::lock_guard<std::mutex> lock(send_mutex);
            sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
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

        void send_danger_zone(double radius_m, int32_t lat_e7, int32_t lon_e7, float confidence, uint16_t group_id) {
            if (sock_fd < 0){
                //TODO: update with LOG
                std::cout << "[MAVLINK] sock_fd is less then 0." << std::endl;
                return;
            } 

            mavlink_message_t msg;
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

            // 1. Pack the circular fence exclusion item
            mavlink_msg_mission_item_int_pack(
                system_id,
                component_id,
                &msg,
                0,                                  // target_system (0 = broadcast to all GCS)
                0,                                  // target_component
                group_id,                           // seq (maps to group_id / item index)
                MAV_FRAME_GLOBAL_INT,               // coordinate frame with 1e7 scaling
                MAV_CMD_NAV_FENCE_CIRCLE_EXCLUSION, // Command: 5003
                0,                                  // current
                1,                                  // autocontinue
                static_cast<float>(radius_m),       // Param 1: Radius (m)
                0.0f,                               // Param 2: 0 = Exclusion zone RED
                confidence,                         // Param 3: Confidence rating
                static_cast<float>(group_id),       // Param 4: Group identifier
                lat_e7,                             // x: Latitude (degE7)
                lon_e7,                             // y: Longitude (degE7)
                0.0f,                               // z: Altitude is out of scope
                MAV_MISSION_TYPE_FENCE              // mission_type: 2
            );

            uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
                std::cout << "[MAVLINK] circular fence exclusion item has been sent" << std::endl;
            }

                // 2. Broadcast a STATUSTEXT notification so QGC's alert console shows the mine detection info
                char text[50];
                std::snprintf(text, sizeof(text), "[ALERT] Mine #%u detected! Conf: %.0f%%", group_id, confidence * 100.0f);
                
                mavlink_msg_statustext_pack(
                    system_id,
                    component_id,
                    &msg,
                    MAV_SEVERITY_CRITICAL,              // Red critical alert banner in QGC
                    text,
                    0,
                    0
                );

                len = mavlink_msg_to_send_buffer(buffer, &msg);
                {
                    std::lock_guard<std::mutex> lock(send_mutex);
                    sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
                    std::cout << "[MAVLINK] STATUSTEXT has been sent" << std::endl;
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