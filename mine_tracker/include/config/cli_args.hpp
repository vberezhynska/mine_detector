#pragma once

namespace mine_tracker {

    inline constexpr double DEFAULT_FLAG_RADIUS = 30.0;

    double parse_flag_radius(int argc, char* argv[], double default_radius = DEFAULT_FLAG_RADIUS);

} // namespace mine_tracker