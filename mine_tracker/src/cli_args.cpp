#include "config/cli_args.hpp"

#include <charconv>
#include <cstring>
#include <iostream>
#include <string_view>

namespace mine_tracker {

namespace {

    bool parse_double_positive(std::string_view str, double& out_val) {
        if (str.empty()) return false;

        double val = 0.0;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);

        // ec == std::errc{} means success; ptr reaching end means the full string was valid
        if (ec == std::errc{} && ptr == str.data() + str.size() && val > 0.0) {
            out_val = val;
            return true;
        }
        return false;
    }

} // namespace

double parse_flag_radius(int argc, char* argv[], double default_radius) {
    double radius = default_radius;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);

        // Form 1: --radius <val>
        if (arg == "--radius") {
            if (i + 1 < argc) {
                std::string_view val_str(argv[++i]);
                if (!parse_double_positive(val_str, radius)) {
                    std::cerr << "Warning: Invalid --radius value '" << val_str 
                              << "'. Falling back to " << default_radius << " m\n";
                    radius = default_radius;
                }
            } else {
                std::cerr << "Warning: --radius passed without a value. Using " 
                          << default_radius << " m\n";
            }
            break;
        }

        // Form 2: --radius=<val>
        constexpr std::string_view prefix = "--radius=";
        if (arg.starts_with(prefix)) {
            std::string_view val_str = arg.substr(prefix.size());
            if (!parse_double_positive(val_str, radius)) {
                std::cerr << "Warning: Invalid --radius value '" << val_str 
                          << "'. Falling back to " << default_radius << " m\n";
                radius = default_radius;
            }
            break;
        }
    }

    std::cout << "[Config] Flag radius: " << radius << " m\n";
    return radius;
}

} // namespace mine_tracker