#pragma once

#include <cstdint>

namespace mine_detector {
    enum class GPType : uint8_t {
        GPGGA = 0, //best one
        GPRMC = 1,
        GPGLL = 2,
        UNKNOWN = 255
    };

    struct GpsSystemFixData {
        GPType gp_type = GPType::UNKNOWN; 
        bool fix_valid{false}; //false: module is searching for satellites
        double latitude{0.0};       
        double longitude{0.0};      
        double altitude{0.0f};
        float hdop{99.9f}; // hdop < 1 => 1-2 m | 1 <= hdop < 2 => up to 5 m | 2 <= hdop < 5 => more then 5 m | hdop > 5 => very poor
        uint8_t satellite_count{0};
    };

    class GgaDecoder {
        public:
            static bool parse(const char* raw_data, GpsSystemFixData& outPos);
            static double to_double_point(const int32_t value);
            static int32_t to_int32_point(const double val);

        private:
            static bool validate_data_quiality(char* tokens[15], int tokenCount, GpsSystemFixData& outPos);
    };
} //namespace mine_detector