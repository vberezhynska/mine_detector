#include <gtest/gtest.h>
#include "flags.hpp"

namespace mine_tracker {
namespace {
//Test 1: Reject point 0.00, 0.00
TEST(FlagsTest, RejectCoordinatesThatAreCloseToZeroZeroPoint) {
    constexpr double radius_meters = 50.0;
    Flags tracker(radius_meters);

    EXPECT_EQ(tracker.add_flag(0.005, 10.0), -1);
    EXPECT_EQ(tracker.add_flag(10.0, 0.005), -1);
}

//Test 2: Create a one group from 
TEST(FlagsTest, SingleGroupCreation) {
    constexpr double radius_meters = 50.0;
    Flags tracker(radius_meters);

    // 1. Base point in Kochany, Kherson region: ~46.521800° N, 32.610500° E (lon, lat)
    const double base_lon = 32.610500;
    const double base_lat = 46.521800;

    // First valid flag has no nearby neighbors -> solitary candidate (returns -1)
    int first_res = tracker.add_flag(base_lon, base_lat);
    EXPECT_EQ(first_res, -1);
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, base_lat));

    // 2. Add a nearby point (~10m east, delta lon ≈ 0.000131° at lat ~46.52°)
    // Should promote solitary flag into a new group (group_id = 0)
    const double nearby_lon_1 = base_lon + 0.000131;
    const double nearby_lat_1 = base_lat;

    int group_id = tracker.add_flag(nearby_lon_1, nearby_lat_1);
    EXPECT_EQ(group_id, 0);

    // 3. Add a third point close to the existing group (~10m north, delta lat ≈ 0.000090°)
    // Should corroborate existing cluster and return the same group_id
    const double nearby_lon_2 = base_lon;
    const double nearby_lat_2 = base_lat + 0.000090;

    int second_group_hit = tracker.add_flag(nearby_lon_2, nearby_lat_2);
    EXPECT_EQ(second_group_hit, 0);

    // 4. Query point far away (~100km north toward Kryvyi Rih / Dnipro basin)
    // Should not match any flags in Kochany
    const double far_lon = 32.610500;
    const double far_lat = 47.500000;
    EXPECT_FALSE(tracker.is_within_radius_of_any(far_lon, far_lat));

    // Far point added as solitary candidate -> returns -1
    EXPECT_EQ(tracker.add_flag(far_lon, far_lat), -1);
    EXPECT_TRUE(tracker.is_within_radius_of_any(far_lon, far_lat));
}

//Test 3: Two points 51m apart (both solitary), bridged by a third point
TEST(FlagsTest, BridgingTwoPointsFiftyOneMetersApart) {
    constexpr double radius_meters = 50.0;
    Flags tracker(radius_meters);

    // At lat ~46.52° N (Kochany, Kherson region):
    // 1 degree latitude ≈ 111,195 meters -> 1 meter ≈ 0.00000899 degrees
    constexpr double meter_to_lat = 0.00000899;

    const double base_lon = 32.610500;
    const double pA_lat   = 46.521800;

    // 1. Point A: First point, inserted as a solitary flag
    int res_A = tracker.add_flag(base_lon, pA_lat);
    EXPECT_EQ(res_A, -1);
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, pA_lat));

    // 2. Point B: 51 meters north of Point A (exceeds the 50m radius)
    // Distance = 51.0 * meter_to_lat ≈ 0.00045849°
    const double pB_lat = pA_lat + (51.0 * meter_to_lat);
    int res_B = tracker.add_flag(base_lon, pB_lat);

    // Must be solitary (-1) because 51m > 50m radius
    EXPECT_EQ(res_B, -1);
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, pB_lat));

    // 3. Point C: Placed between A and B
    // 24 meters north of Point A, meaning 27 meters south of Point B
    // Both are within 50m, but Point A is the nearest neighbor (24m < 27m)
    const double pC_lat = pA_lat + (24.0 * meter_to_lat);
    int res_C = tracker.add_flag(base_lon, pC_lat);

    // Should successfully match Point A and create group 0
    EXPECT_EQ(res_C, 0);

    // Verify all three locations are recognized within radius of flags/centroids in the R-tree
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, pA_lat));
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, pB_lat));
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, pC_lat));
}

//Test 4: Multiple groups
TEST(FlagsTest, CreatesThreeDistinctGroupsAndRetainsSolitaryOutliers) {
    constexpr double radius_meters = 50.0;
    Flags tracker(radius_meters);

    // At lat ~46.52° N (Kochany, Kherson region):
    // 1 meter latitude  ≈ 0.00000899 degrees
    // 1 meter longitude ≈ 0.00001309 degrees (1 / (111320 * cos(46.52°)))
    constexpr double meter_to_lat = 0.00000899;
    constexpr double meter_to_lon = 0.00001309;

    // Base coordinates in Kochany
    const double base_lon = 32.610500;
    const double base_lat = 46.521800;

    // =========================================================================
    // Group 0: Formed around the base origin
    // =========================================================================
    // First point -> Solitary
    EXPECT_EQ(tracker.add_flag(base_lon, base_lat), -1);

    // Second point ~15m North -> Pairs with first point, creating Group 0
    const double g0_p2_lat = base_lat + (15.0 * meter_to_lat);
    EXPECT_EQ(tracker.add_flag(base_lon, g0_p2_lat), 0);

    // =========================================================================
    // Group 1: Formed ~300m East (well outside Group 0's 50m radius)
    // =========================================================================
    const double g1_lon = base_lon + (300.0 * meter_to_lon);
    const double g1_lat = base_lat;

    // First point -> Solitary
    EXPECT_EQ(tracker.add_flag(g1_lon, g1_lat), -1);

    // Second point ~20m East -> Pairs with preceding point, creating Group 1
    const double g1_p2_lon = g1_lon + (20.0 * meter_to_lon);
    EXPECT_EQ(tracker.add_flag(g1_p2_lon, g1_lat), 1);

    // =========================================================================
    // Group 2: Formed ~300m North (well outside Group 0 and Group 1)
    // =========================================================================
    const double g2_lon = base_lon;
    const double g2_lat = base_lat + (300.0 * meter_to_lat);

    // First point -> Solitary
    EXPECT_EQ(tracker.add_flag(g2_lon, g2_lat), -1);

    // Second point ~10m North -> Pairs with preceding point, creating Group 2
    const double g2_p2_lat = g2_lat + (10.0 * meter_to_lat);
    EXPECT_EQ(tracker.add_flag(g2_lon, g2_p2_lat), 2);

    // =========================================================================
    // Solitary Outliers: Placed far from all clusters and each other
    // =========================================================================
    // Outlier 1: ~150m East, ~150m North (in the gap between G0, G1, and G2)
    const double outlier1_lon = base_lon + (150.0 * meter_to_lon);
    const double outlier1_lat = base_lat + (150.0 * meter_to_lat);
    EXPECT_EQ(tracker.add_flag(outlier1_lon, outlier1_lat), -1);

    // Outlier 2: ~300m West
    const double outlier2_lon = base_lon - (300.0 * meter_to_lon);
    const double outlier2_lat = base_lat;
    EXPECT_EQ(tracker.add_flag(outlier2_lon, outlier2_lat), -1);

    // Outlier 3: ~300m South
    const double outlier3_lon = base_lon;
    const double outlier3_lat = base_lat - (300.0 * meter_to_lat);
    EXPECT_EQ(tracker.add_flag(outlier3_lon, outlier3_lat), -1);

    // =========================================================================
    // Proximity Verifications
    // =========================================================================
    // Group centroids/points must register as within radius
    EXPECT_TRUE(tracker.is_within_radius_of_any(base_lon, base_lat));
    EXPECT_TRUE(tracker.is_within_radius_of_any(g1_lon, g1_lat));
    EXPECT_TRUE(tracker.is_within_radius_of_any(g2_lon, g2_lat));

    // Solitary outliers are in the R-tree, so their exact coordinates must match
    EXPECT_TRUE(tracker.is_within_radius_of_any(outlier1_lon, outlier1_lat));
    EXPECT_TRUE(tracker.is_within_radius_of_any(outlier2_lon, outlier2_lat));
    EXPECT_TRUE(tracker.is_within_radius_of_any(outlier3_lon, outlier3_lat));

    // Arbitrary unvisited coordinate far away (~1km away) must return false
    const double unvisited_lon = base_lon + (1000.0 * meter_to_lon);
    const double unvisited_lat = base_lat + (1000.0 * meter_to_lat);
    EXPECT_FALSE(tracker.is_within_radius_of_any(unvisited_lon, unvisited_lat));
}
} // namespace
} // namespace mine_tracker