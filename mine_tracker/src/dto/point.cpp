#include "dto/point.hpp"

#include <cstdint>
const uint32_t SCALE_FACTOR = 1000000;

namespace Location {
    void Point::setFromInt(uint32_t lat, uint32_t lon){
        LAT = (uint32_t)(lat * SCALE_FACTOR);
        LON = (uint32_t)(lon * SCALE_FACTOR);
    };
}