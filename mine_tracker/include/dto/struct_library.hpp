#pragma once

#include <string>
#include <cstdint>

namespace mine_tracker {

    inline constexpr double SCALE_FACTOR = 1000000.0;

    struct MineAlertData {
        std::string event;
        int32_t lat_int{0};
        int32_t lon_int{0};
        int8_t gp_type{0};

        static constexpr int32_t to_int32_point(double val) noexcept {
            return static_cast<int32_t>(val * SCALE_FACTOR);
        }

        static constexpr double to_double_point(int32_t value) noexcept {
            return static_cast<double>(value) / SCALE_FACTOR;
        }

        [[nodiscard]] double lat() const noexcept {
            return to_double_point(lat_int);
        }

        [[nodiscard]] double lon() const noexcept {
            return to_double_point(lon_int);
        }
    };

} // namespace mine_tracker