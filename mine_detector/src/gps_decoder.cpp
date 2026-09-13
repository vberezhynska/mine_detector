#include "gps_decoder.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#ifdef HOST_TESTING
  #include <cstdio>
  #define ESP_LOGI(tag, fmt, ...) printf("[INFO] [%s] " fmt "\n", tag, ##__VA_ARGS__)
  #define ESP_LOGW(tag, fmt, ...) printf("[WARN] [%s] " fmt "\n", tag, ##__VA_ARGS__)
  #define ESP_LOGE(tag, fmt, ...) printf("[ERR]  [%s] " fmt "\n", tag, ##__VA_ARGS__)
  #define ESP_LOGD(tag, fmt, ...) printf("[DEBUG]  [%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #include "esp_log.h"
#endif

#include "external/telemetry_types.hpp"

static const char* TAG = "GPS_DECODER";

namespace mine_detector {
    const uint32_t SCALE_FACTOR = 1000000;

    int32_t GgaDecoder::to_int32_point(double val) {
        return static_cast<int32_t>(val * SCALE_FACTOR);
    }
    
    double GgaDecoder::to_double_point(const int32_t value){
        return static_cast<double>(value) / SCALE_FACTOR;
    }

    bool GgaDecoder::validate_nmea_checksum(const char* line) {
        if (line == nullptr || line[0] != '$') return false;

        // Find the '*' delimiter
        const char* star = strchr(line, '*'); // Needs at least *XX
        if (!star || strlen(star) < 3) return false;

        // Calculate XOR checksum of characters between '$' and '*'
        uint8_t calculated_checksum = 0;
        for (const char* p = line + 1; p < star; ++p) {
            calculated_checksum ^= static_cast<uint8_t>(*p);
        }

        // Convert hex string after '*' to integer
        char hex_str[3] = { star[1], star[2], '\0' };
        uint8_t expected_checksum = static_cast<uint8_t>(strtol(hex_str, nullptr, 16));

        return (calculated_checksum == expected_checksum);
    }

    bool GgaDecoder::validate_gga_data_quiality(char* tokens[15], GpsSystemFixData& outPos){
        // Field 6: Fix Status (0 = Invalid, 1 = GPS Fix, 2 = DGPS)
        int fixQuality = atoi(tokens[6]); //ASCII to Integer
        outPos.fix_valid = (fixQuality > 0);

        if (!outPos.fix_valid) {
            return false;
        }

        // Field 8:: Drop poor quality readings (> 2.0 means high noise/drift)
        if (strlen(tokens[8]) > 0) {
            outPos.hdop = static_cast<float>(std::strtof(tokens[8], nullptr));
        } else {
            outPos.hdop = 99.9f;
        }

        if (outPos.hdop > 2.0f) {
            ESP_LOGW(TAG, "Dropped reading due to high HDOP: %.2f", outPos.hdop);
            outPos.fix_valid = false;
            return false; 
        }

        // Field 7: Satellites -> uint8_t
        auto satellite_count = static_cast<uint8_t>(atoi(tokens[7]));
        if (satellite_count > 35)
        {
            ESP_LOGW("GPS", "Corrupted satellite count detected: %d. Dropping packet.", satellite_count);
            outPos.fix_valid = false;
            return false;
        }

        outPos.satellite_count = satellite_count;

        return true;
    }

    // Latitude (DDMM.MMMM) and Direction (N/S) -> double
    // Longitude (DDDMM.MMMM) and Direction (E/W) -> double
    double GgaDecoder::parse_coordinate(const char* val_str, const char* dir_str) {
        if (!val_str || !dir_str || strlen(val_str) == 0 || strlen(dir_str) == 0) {
            return 0.0;
        }

        double raw = std::strtod(val_str, nullptr);
        int degrees = static_cast<int>(raw / 100.0);
        double minutes = raw - (degrees * 100.0);
        double decimal_degrees = degrees + (minutes / 60.0);

        if (dir_str[0] == 'S' || dir_str[0] == 'W' || dir_str[0] == 's' || dir_str[0] == 'w') {
            decimal_degrees = -decimal_degrees;
        }

        return decimal_degrees;
    }

    uint8_t GgaDecoder::tokenize(const uint8_t token_length, char* tokens[], char* buffer) {
        uint8_t tokenCount = 0; 
        char* ptr = buffer;

        while (ptr && tokenCount < token_length) {
            tokens[tokenCount++] = ptr;
            char* comma = strchr(ptr, ',');
            if (comma) {
                *comma = '\0';
                ptr = comma + 1;
            } else {
                ptr = nullptr;
            }
        }
        
        return tokenCount;
    }

    bool GgaDecoder::parse_gga(const char* ggaStart, GpsSystemFixData& outPos){
        const uint8_t token_length = 15;
        const uint8_t n_critical_fields = 10;
        
        if (!validate_nmea_checksum(ggaStart)){
            ESP_LOGW(TAG, "$GPGGA checksum is wrong.");
            return false;
        }
        
        char* tokens[token_length]; 
        char buffer[128];
        strncpy(buffer, ggaStart, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0'; 
        auto t_count = tokenize(token_length, tokens, buffer);

        if (t_count < n_critical_fields)
            return false; //Critical fields: Lat (2/3), Lon (4/5), Fix (6), Sats (7), HDOP (8), Alt (9)

        if (!validate_gga_data_quiality(tokens, outPos))
            return false;

        outPos.gp_type = mine_tracker::NMEA_Type::GPGGA;
        if (strlen(tokens[2]) > 0 && strlen(tokens[3]) > 0) {
            outPos.latitude = parse_coordinate(tokens[2], tokens[3]);
            outPos.latitude_int = to_int32_point(outPos.latitude);
        }
        
        if (strlen(tokens[4]) > 0 && strlen(tokens[5]) > 0) {
            outPos.longitude = parse_coordinate(tokens[4], tokens[5]);
            outPos.longitude_int = to_int32_point(outPos.longitude);
        }

        // Field 9: Altitude -> double
        if (strlen(tokens[9]) > 0) {
            outPos.altitude = std::strtod(tokens[9], nullptr);
        }

        return true;
    }

    bool GgaDecoder::parse_rmc(const char* rmcStart, GpsSystemFixData& outPos){
        const uint8_t token_length = 13;
        const uint8_t n_critical_fields = 7;
        
        if (!validate_nmea_checksum(rmcStart)){
            ESP_LOGW(TAG, "$GPRMC checksum is wrong.");
            return false;
        }
        
        char buffer[128];
        strncpy(buffer, rmcStart, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        char* tokens[token_length];
        auto t_count = tokenize(token_length, tokens, buffer);
        if (t_count < n_critical_fields) // (2) = Status, (3/4) Lat, (5/6) - Long
            return false;

        // Field 2: Status ('A' = Active/Valid, 'V' = Void/Invalid)
        if (tokens[2][0] != 'A') {
            outPos.fix_valid = false;
            return false;
        }

        outPos.gp_type = mine_tracker::NMEA_Type::GPRMC;
        // Parse Coordinates
        if (strlen(tokens[3]) > 0 && strlen(tokens[4]) > 0) {
            outPos.latitude = parse_coordinate(tokens[3], tokens[4]);
            outPos.latitude_int = to_int32_point(outPos.latitude);
        }

        if (strlen(tokens[5]) > 0 && strlen(tokens[6]) > 0) {
            outPos.longitude = parse_coordinate(tokens[5], tokens[6]);
            outPos.longitude_int = to_int32_point(outPos.longitude);
        }

        outPos.fix_valid  = true;
        return true;               
    }

    bool GgaDecoder::parse_gll(const char* gllStart, GpsSystemFixData& outPos){
        const uint8_t token_length = 7;
        const uint8_t n_critical_fields = 6;
        
        if (!validate_nmea_checksum(gllStart)){
            ESP_LOGW(TAG, "$GPGLL checksum is wrong.");
            return false;
        }
        
        char buffer[128];
        strncpy(buffer, gllStart, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        char* tokens[token_length];
        auto t_count = tokenize(token_length, tokens, buffer);
        if (t_count < n_critical_fields)
            return false;

        // 'V' (Void/Invalid)
        if (tokens[6][0] != 'A') {
            outPos.fix_valid = false;
            return false;
        }

        outPos.gp_type = mine_tracker::NMEA_Type::GPGLL;
        // Parse Coordinates
        if (strlen(tokens[1]) > 0 && strlen(tokens[2]) > 0) {
            outPos.latitude = parse_coordinate(tokens[1], tokens[2]);
            outPos.latitude_int = to_int32_point(outPos.latitude);
        }

        if (strlen(tokens[3]) > 0 && strlen(tokens[4]) > 0) {
            outPos.longitude = parse_coordinate(tokens[3], tokens[4]);
            outPos.longitude_int = to_int32_point(outPos.longitude);
        }

        outPos.fix_valid = true;
        return true;
    }

    bool GgaDecoder::parse(const char* raw_data, GpsSystemFixData& outPos) {
        if (raw_data == nullptr) return false;

        ESP_LOGD(TAG, "RawData received (len: %u): %s", static_cast<unsigned int>(strlen(raw_data)), raw_data);

        char* ggaStart = strstr(const_cast<char*>(raw_data), "$GPGGA");
        if (ggaStart != nullptr && parse_gga(ggaStart, outPos)) {
            return true; //Parsed with GPGGA
        }
        ESP_LOGI(TAG, "No $GPGGA header in current buffer slice. Falling back to other types.");

        char* rmsStart = strstr(const_cast<char*>(raw_data), "$GPRMC");
        if (rmsStart != nullptr && parse_rmc(rmsStart, outPos)) {
            return true; //Parsed with GPRMC
        }
        ESP_LOGI(TAG, "No $GPRMC header in current buffer slice. Falling back to other types.");

        char* gllStart = strstr(const_cast<char*>(raw_data), "$GPGLL");
        if (gllStart != nullptr && parse_gll(gllStart, outPos)) {
            return true; //Parsed with GPGLL
        }

        ESP_LOGW(TAG, "None of $GPGGA $GPRMC $GPGLL headers found.");
        return false;
    }

} // namespace mine_detector