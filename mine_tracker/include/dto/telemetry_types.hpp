#pragma once

#include <cstdint>
#include <iostream>
#include <variant>

const uint32_t GPS_SCALE_FACTOR = 1000000; 

namespace mine_tracker {

    enum class NMEA_Type : uint8_t {
        GPGGA = 0, //best one
        GPRMC = 1,
        GPGLL = 2,
        UNKNOWN = 255
    };

    constexpr const char* to_string(NMEA_Type type) {
        switch (type) {
            case NMEA_Type::GPGGA:   return "GPGGA";
            case NMEA_Type::GPRMC:   return "GPRMC";
            case NMEA_Type::GPGLL:   return "GPGLL";
            case NMEA_Type::UNKNOWN: 
            default:                 return "UNKNOWN";
        }
    }

enum class MessageType : uint8_t {
    TELEMETRY  = 0x01
};

struct __attribute__((__packed__)) TelemetryPayload {
    uint8_t  msg_type = static_cast<uint8_t>(MessageType::TELEMETRY);
    uint32_t latitude;   // Scaled integer (e.g. 50451200)
    uint32_t longitude;  // Scaled integer (e.g. 30523400)
    uint8_t  gps_type;   // 0 = No Fix, 1 = 2D, 2 = 3D
    uint32_t timestamp;  // Epoch time or uptime ms
};

using UdpPacket = std::variant<std::monostate, TelemetryPayload>;

[[nodiscard]] constexpr double to_gps(int32_t lat_long) noexcept {
    return static_cast<double>(lat_long) / GPS_SCALE_FACTOR;
}

} // namespace mine_tracker