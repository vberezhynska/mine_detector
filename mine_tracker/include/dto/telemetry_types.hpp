#pragma once

#include <cstdint>
#include <iostream>
#include <variant>

namespace mine_tracker {

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


} // namespace mine_tracker