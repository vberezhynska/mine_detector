#include "gps_decoder.hpp"
#include "esp_log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

static const char* TAG = "GPS_DECODER";

namespace mine_detector {

    bool GgaDecoder::parse(const char* raw_data, GpsSystemFixData& outPos) {
        if (raw_data == nullptr) return false;

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

        if (tokenCount < 10) return false;

        // Field 6: Fix Status (0 = Invalid, 1 = GPS Fix, 2 = DGPS)
        int fixQuality = atoi(tokens[6]);
        outPos.fixValid = (fixQuality > 0);

        if (!outPos.fixValid) {
            return false; // No lock yet
        }

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