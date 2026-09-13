#pragma once

#include <cstdint>

namespace mine_detector {
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

    struct GpsSystemFixData {
        NMEA_Type gp_type = NMEA_Type::UNKNOWN; 
        bool fix_valid{false}; //false: module is searching for satellites
        uint32_t latitude_int{0};       
        uint32_t longitude_int{0}; 
        double latitude{0.0f};       
        double longitude{0.0f};      
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
            static bool validate_nmea_checksum(const char* line);
            static uint8_t tokenize(const uint8_t token_length, char* tokens[], char* buffer);  
            static bool validate_gga_data_quiality(char* tokens[15], GpsSystemFixData& outPos);
            static double parse_coordinate(const char* val_str, const char* dir_str);
            static bool parse_gga(const char* ggaStart, GpsSystemFixData& outPos);
            static bool parse_rmc(const char* rmcStart, GpsSystemFixData& outPos);
            static bool parse_gll(const char* gllStart, GpsSystemFixData& outPos);
    };
} //namespace mine_detector