#pragma once

#include <cstdint>

namespace Location {
    struct Point {
        double LAT; 
        double LON;

        void setFromInt(uint32_t lat, uint32_t lon);
    };
}//namespace Location