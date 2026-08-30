#include "gps_decoder.hpp"

#include "esp_log.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

static const char* TAG = "GPS_DECODER";

namespace mine_detector {
    const uint32_t SCALE_FACTOR = 1000000;

    int32_t GgaDecoder::to_int32_point(double val) {
        return static_cast<int32_t>(val * SCALE_FACTOR);
    }
    
    double GgaDecoder::to_double_point(const int32_t value){
        return static_cast<double>(value) / SCALE_FACTOR;
    }

    bool GgaDecoder::validate_data_quiality(char* tokens[15], int tokenCount, GpsSystemFixData& outPos){
        if (tokenCount < 10) 
            return false;

        // Field 6: Fix Status (0 = Invalid, 1 = GPS Fix, 2 = DGPS)
        int fixQuality = atoi(tokens[6]);
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
            ESP_LOGD(TAG, "Dropped reading due to high HDOP: %.2f", outPos.hdop);
            outPos.fix_valid = false;
            return false; 
        }

        return true;
    }

    bool GgaDecoder::parse(const char* raw_data, GpsSystemFixData& outPos) {
        if (raw_data == nullptr) return false;

        ESP_LOGI(TAG, "RawData received (len: %u): %s", static_cast<unsigned int>(strlen(raw_data)), raw_data);

        char* ggaStart = strstr(const_cast<char*>(raw_data), "$GPGGA");
        if (ggaStart == nullptr || ggaStart[0] == '\0') {
            ESP_LOGW(TAG, "No $GPGGA header in current buffer slice");
            return false;
        }

        // Copy starting from the $GPGGA pointer
        char buffer[128];
        strncpy(buffer, ggaStart, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        char* tokens[15];
        int tokenCount = 0;
        char* ptr = buffer;

        while (ptr && tokenCount < 15) {
            tokens[tokenCount++] = ptr;
            char* comma = strchr(ptr, ',');
            if (comma) {
                *comma = '\0';
                ptr = comma + 1;
            } else {
                ptr = nullptr;
            }
        }

        outPos.gp_type = GPType::GPGGA;
        if (!validate_data_quiality(tokens, tokenCount, outPos))
            return false;

        // Field 2 & 3: Latitude (DDMM.MMMM) and Direction (N/S) -> double
        if (strlen(tokens[2]) > 0 && strlen(tokens[3]) > 0) {
            double rawLat = std::strtod(tokens[2], nullptr);
            int degrees = static_cast<int>(rawLat / 100.0);
            double minutes = rawLat - (degrees * 100.0);
            outPos.latitude = degrees + (minutes / 60.0);
            if (tokens[3][0] == 'S') outPos.latitude = -outPos.latitude;
        }

        // Field 4 & 5: Longitude (DDDMM.MMMM) and Direction (E/W) -> double
        if (strlen(tokens[4]) > 0 && strlen(tokens[5]) > 0) {
            double rawLon = std::strtod(tokens[4], nullptr);
            int degrees = static_cast<int>(rawLon / 100.0);
            double minutes = rawLon - (degrees * 100.0);
            outPos.longitude = degrees + (minutes / 60.0);
            if (tokens[5][0] == 'W') outPos.longitude = -outPos.longitude;
        }

        // Field 7: Satellites -> uint8_t
        outPos.satellite_count = static_cast<uint8_t>(atoi(tokens[7]));

        // Field 9: Altitude -> double
        if (strlen(tokens[9]) > 0) {
            outPos.altitude = std::strtod(tokens[9], nullptr);
        }

        return true;
    }

} // namespace mine_detector