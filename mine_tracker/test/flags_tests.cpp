#include <gtest/gtest.h>
#include "flags.hpp"

namespace mine_tracker {
namespace {

TEST(FlagsTest, HandlesSolitaryFlagGroupingCentroidShiftAndProximity) {
    constexpr double radius_meters = 50.0;
    Flags tracker(radius_meters);

    // 1. Coordinates below 0.01 threshold should be rejected
    EXPECT_EQ(tracker.add_flag(0.005, 10.0), -1);
    EXPECT_EQ(tracker.add_flag(10.0, 0.005), -1);

    // 2. Base point in Kochany, Kherson region: ~46.521800° N, 32.610500° E (lon, lat)
    const double base_lon = 32.610500;
    const double base_lat = 46.521800;

    // First valid flag has no nearby neighbors -> solitary candidate (returns -1)
    int first_res = tracker.add_flag(base_lon, base_lat);
    EXPECT_EQ(first_res, -1);
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, base_lat));

    // 3. Add a nearby point (~10m east, delta lon ≈ 0.000131° at lat ~46.52°)
    // Should promote solitary flag into a new group (group_id = 0)
    const double nearby_lon_1 = base_lon + 0.000131;
    const double nearby_lat_1 = base_lat;

    int group_id = tracker.add_flag(nearby_lon_1, nearby_lat_1);
    EXPECT_EQ(group_id, 0);

    // 4. Add a third point close to the existing group (~10m north, delta lat ≈ 0.000090°)
    // Should corroborate existing cluster and return the same group_id
    const double nearby_lon_2 = base_lon;
    const double nearby_lat_2 = base_lat + 0.000090;

    int second_group_hit = tracker.add_flag(nearby_lon_2, nearby_lat_2);
    EXPECT_EQ(second_group_hit, 0);

    // 5. Query point far away (~100km north toward Kryvyi Rih / Dnipro basin)
    // Should not match any flags in Kochany
    const double far_lon = 32.610500;
    const double far_lat = 47.500000;
    EXPECT_FALSE(tracker.is_within_radius_of_any(far_lon, far_lat));

    // Far point added as solitary candidate -> returns -1
    EXPECT_EQ(tracker.add_flag(far_lon, far_lat), -1);
    EXPECT_TRUE(tracker.is_within_radius_of_any(far_lon, far_lat));
}

} // namespace
} // namespace mine_tracker