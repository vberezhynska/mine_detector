#include <gtest/gtest.h>
#include "gps_decoder.hpp"

namespace mine_detector {

class GgaDecoderTest : public ::testing::Test {
protected:
    GgaDecoder decoder;
    GpsSystemFixData fix_data{};

    void SetUp() override {
        // Reset output struct before each test
        fix_data = GpsSystemFixData{};
    }
};

// Test 1: Valid $GPGGA Sentence Parsing
TEST_F(GgaDecoderTest, ParsesValidGpggaSentenceSuccessfully) {
    // Standard $GPGGA sentence with 8 satellites, HDOP 0.9, Alt 545.4m
    const char* valid_gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    bool success = decoder.parse(valid_gga, fix_data);

    EXPECT_TRUE(success);
    EXPECT_TRUE(fix_data.fix_valid);
    EXPECT_EQ(fix_data.gp_type, mine_tracker::NMEA_Type::GPGGA);
    EXPECT_NEAR(fix_data.latitude, 48.1173, 0.0001);   // 48 deg + 7.038/60 min
    EXPECT_NEAR(fix_data.longitude, 11.5166, 0.0001);  // 11 deg + 31.000/60 min
    EXPECT_EQ(fix_data.satellite_count, 8);
    EXPECT_FLOAT_EQ(fix_data.hdop, 0.9f);
    EXPECT_DOUBLE_EQ(fix_data.altitude, 545.4);
}

// Test 2: Invalid Checksum Rejection
TEST_F(GgaDecoderTest, RejectsInvalidChecksum) {
    // Checksum changed from *47 to *99
    const char* bad_checksum_gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*99";

    bool success = decoder.parse(bad_checksum_gga, fix_data);

    EXPECT_FALSE(success);
    EXPECT_FALSE(fix_data.fix_valid);
}

// Test 3: High HDOP Filtering (> 2.0)
TEST_F(GgaDecoderTest, DropsReadingWithHighHdop) {
    // HDOP is 2.5 (above 2.0 limit)
    const char* high_hdop_gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,2.5,545.4,M,46.9,M,,*4B";

    bool success = decoder.parse(high_hdop_gga, fix_data);

    EXPECT_FALSE(success);
    EXPECT_FALSE(fix_data.fix_valid);
}

// Test 4: Corrupted Satellite Count Guard (> 35)
TEST_F(GgaDecoderTest, RejectsCorruptedSatelliteCount) {
    // Satellites set to 99
    const char* corrupt_sats_gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,99,0.9,545.4,M,46.9,M,,*47";

    bool success = decoder.parse(corrupt_sats_gga, fix_data);

    EXPECT_FALSE(success);
    EXPECT_FALSE(fix_data.fix_valid);
}

// Test 5: Fallback to $GPRMC when $GPGGA is absent
TEST_F(GgaDecoderTest, FallsBackToGprmcWhenGgaNotPresent) {
    // Valid $GPRMC Active ('A') sentence
    const char* valid_rmc = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";

    bool success = decoder.parse(valid_rmc, fix_data);

    EXPECT_TRUE(success);
    EXPECT_TRUE(fix_data.fix_valid);
    EXPECT_EQ(fix_data.gp_type, mine_tracker::NMEA_Type::GPRMC);
    EXPECT_NEAR(fix_data.latitude, 48.1173, 0.0001);
    EXPECT_NEAR(fix_data.longitude, 11.5166, 0.0001);
}

} // namespace mine_detector