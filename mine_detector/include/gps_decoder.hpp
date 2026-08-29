#pragma once

#include <cstdint>

namespace mine_detector {
    struct GpsSystemFixData {
            bool fixValid{false}; //false: module is searching for satellites
            double latitude{0.0};       
            double longitude{0.0};      
            double altitude{0.0f};       
            uint8_t satellite_count{0};
    };

    class GgaDecoder {
        public:
            static bool parse(const char* raw_data, GpsSystemFixData& outPos);
    };
} //namespace mine_detector