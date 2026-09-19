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
                uint32_t icao_id;   // Unique 24-bit ICAO address for each mine
                uint16_t group_id;
                int32_t lat_e7;
                int32_t lon_e7;
            };
            std::mutex mines_mutex;
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

            // 1. Primary Vehicle Heartbeat (SysID = 1, Rover)
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

            // 2. Snapshot all registered mines
            std::vector<MineMarker> markers_to_refresh;
            {
                std::lock_guard<std::mutex> lock(mines_mutex);
                markers_to_refresh.reserve(active_mines.size());
                for (const auto& [_, marker] : active_mines) {
                    markers_to_refresh.push_back(marker);
                }
            }

            // 3. Broadcast ADSB_VEHICLE for every active mine (refreshes watchdog timer)
            for (const auto& mine : markers_to_refresh) {
                char callsign[9];
                // Callsign appears as a text label next to the pin in QGC/Mission Planner
                std::snprintf(callsign, sizeof(callsign), "Mine_%03u", mine.group_id);

                mavlink_msg_adsb_vehicle_pack(
                    system_id,
                    component_id,
                    &msg,
                    mine.icao_id,                       // ICAO address (e.g. 0xA000 + group_id)
                    mine.lat_e7,                        // Latitude (degE7)
                    mine.lon_e7,                        // Longitude (degE7)
                    ADSB_ALTITUDE_TYPE_GEOMETRIC,
                    0,                                  // Altitude (mm)
                    0,                                  // Heading (cdeg)
                    0,                                  // Horizontal velocity (cm/s)
                    0,                                  // Vertical velocity (cm/s)
                    callsign,                           // Callsign / Label (max 8 chars + null terminator)
                    ADSB_EMITTER_TYPE_EMERGENCY_SURFACE,
                    1,                                  // tslc: 1 sec since last communication (fresh)
                    ADSB_FLAGS_VALID_COORDS | ADSB_FLAGS_VALID_ALTITUDE | ADSB_FLAGS_VALID_CALLSIGN,
                    7700                                   // Squawk
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
            if (sock_fd < 0) {
                std::cout << "[MAVLINK] sock_fd is less than 0." << std::endl;
                return;
            }

            // Assign a deterministic unique ICAO address (range 0xA000 + group_id)
            uint32_t icao = 0xA000 + (group_id % 1000);

            {
                std::lock_guard<std::mutex> lock(mines_mutex);
                active_mines[group_id] = MineMarker{icao, group_id, lat_e7, lon_e7};
            }

            mavlink_message_t msg;
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

            // 1. Send immediate ADSB_VEHICLE packet
            char callsign[9];
            std::snprintf(callsign, sizeof(callsign), "Mine_%03u", group_id); //TODO: chekc how it's displayed

            mavlink_msg_adsb_vehicle_pack(
                system_id,
                component_id,
                &msg,
                icao,
                lat_e7,
                lon_e7,
                ADSB_ALTITUDE_TYPE_GEOMETRIC,
                0,
                0,
                0,
                0,
                callsign,
                ADSB_EMITTER_TYPE_EMERGENCY_SURFACE,
                0,                                      // tslc: 0s (immediate)
                ADSB_FLAGS_VALID_COORDS | ADSB_FLAGS_VALID_ALTITUDE | ADSB_FLAGS_VALID_CALLSIGN,
                7700
            );

            uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
                std::cout << "[MAVLINK] Placed ADSB Point Obstacle '" << callsign << "' at ("
                        << lat_e7 << ", " << lon_e7 << ")" << std::endl;
            }

            // 2. Send critical STATUSTEXT banner
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

            len = mavlink_msg_to_send_buffer(buffer, &msg);
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                sendto(sock_fd, buffer, len, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
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