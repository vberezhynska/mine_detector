#include <gtest/gtest.h>
#include <cmath>
#include <thread>
#include <chrono>
#include "confidence_engine.hpp"

namespace mine_tracker {
namespace {

// Test 1: Verify construction and default initial state
TEST(ConfidenceEngineTest, InitialStateMatchesHeaderDefaults) {
    GroupConfidence gc(42);

    EXPECT_EQ(gc.group_id, 42);
    EXPECT_EQ(gc.hit_count(), 1); // Starts at 1 per implementation
    EXPECT_DOUBLE_EQ(gc.confidence(), INITIAL_CONFIDENCE);
}

// Test 2: First update_group() call
// Because _last_hit_time was default-initialized (epoch 0),
// is_rapid_touch() is false, applying CONFIDENT_RATE and ACCIDENT_RATE.
TEST(ConfidenceEngineTest, FirstUpdateUsesStandardRates) {
    GroupConfidence gc(1);

    // Initial prior = 0.20
    const double prior = INITIAL_CONFIDENCE;
    const double p_tp = CONFIDENT_RATE;
    const double p_fp = ACCIDENT_RATE;

    const double expected_num = p_tp * prior;
    const double expected_den = expected_num + (p_fp * (1.0 - prior));
    const double expected_conf = expected_num / expected_den;

    gc.update_group();

    EXPECT_EQ(gc.hit_count(), 2);
    EXPECT_NEAR(gc.confidence(), expected_conf, 1e-6);
    EXPECT_GT(gc.confidence(), prior); // 0.85/0.15 boosts belief
}

// Test 3: Rapid consecutive touch uses RAPID_TOUCH_RATE and SEPARATE_TOUCH_RATE
TEST(ConfidenceEngineTest, ImmediateConsecutiveTouchTriggersRapidDebounceRates) {
    GroupConfidence gc(1);

    // Hit 1: standard rates
    gc.update_group();
    const double conf_after_hit1 = gc.confidence();

    // Hit 2: executed immediately (< 5.0 seconds) -> triggers is_rapid_touch == true
    gc.update_group();

    const double p_tp_rapid = RAPID_TOUCH_RATE;
    const double p_fp_rapid = SEPARATE_TOUCH_RATE;

    const double expected_num = p_tp_rapid * conf_after_hit1;
    const double expected_den = expected_num + (p_fp_rapid * (1.0 - conf_after_hit1));
    const double expected_conf = expected_num / expected_den;

    EXPECT_EQ(gc.hit_count(), 3);
    EXPECT_NEAR(gc.confidence(), expected_conf, 1e-6);
}

// Test 4: Confidence increases monotonically under rapid successive hits
TEST(ConfidenceEngineTest, ConfidenceGrowsUnderSuccessiveHits) {
    GroupConfidence gc(10);
    double last_conf = gc.confidence();

    for (int i = 0; i < 4; ++i) {
        gc.update_group();
        EXPECT_GT(gc.confidence(), last_conf);
        EXPECT_LE(gc.confidence(), 1.0);
        last_conf = gc.confidence();
    }

    EXPECT_EQ(gc.hit_count(), 5); // 1 initial + 4 updates
}

// Test 5: Three separate hits with delay 5 sec
TEST(ConfidenceEngineTest, ThreeSpacedHitsReachExpectedConfidence) {
    using namespace std::chrono_literals;

    GroupConfidence gc(1);

    // Initial state: hit_count is 1, prior is 0.20
    EXPECT_EQ(gc.hit_count(), 1);
    EXPECT_DOUBLE_EQ(gc.confidence(), 0.20);

    // Helper lambda to compute one standard Bayesian update step
    auto bayes_step = [](double prior) {
        const double num = CONFIDENT_RATE * prior;
        const double den = num + (ACCIDENT_RATE * (1.0 - prior));
        return num / den;
    };

    const double expected_after_hit1 = bayes_step(INITIAL_CONFIDENCE); // ~0.586207
    const double expected_after_hit2 = bayes_step(expected_after_hit1); // ~0.889230
    const double expected_after_hit3 = bayes_step(expected_after_hit2); // ~0.978490

    // Sleep margin slightly above 5.0s to ensure is_rapid_touch returns false
    const auto sleep_interval = 5100ms;

    // 1. First hit
    gc.update_group();
    EXPECT_EQ(gc.hit_count(), 2);
    EXPECT_NEAR(gc.confidence(), expected_after_hit1, 1e-5);

    // 2. Sleep 5 seconds, then second hit
    std::this_thread::sleep_for(sleep_interval);
    gc.update_group();
    EXPECT_EQ(gc.hit_count(), 3);
    EXPECT_NEAR(gc.confidence(), expected_after_hit2, 1e-5);

    // 3. Sleep 5 seconds, then third hit
    std::this_thread::sleep_for(sleep_interval);
    gc.update_group();
    EXPECT_EQ(gc.hit_count(), 4);
    EXPECT_NEAR(gc.confidence(), expected_after_hit3, 1e-5);

    // Verify it reached ~97.8% confidence
    EXPECT_NEAR(gc.confidence(), 0.978490, 1e-5);
}

// Test 6: Two hits with 3 sec delay and another one with 5 sec delay
TEST(ConfidenceEngineTest, HitFollowedByRapidTouchThenSpacedTouch) {
    using namespace std::chrono_literals;

    GroupConfidence gc(1);

    // Helpers to compute Bayes steps dynamically using class constants
    auto standard_step = [](double prior) {
        const double num = CONFIDENT_RATE * prior;
        const double den = num + (ACCIDENT_RATE * (1.0 - prior));
        return num / den;
    };

    auto rapid_step = [](double prior) {
        const double num = RAPID_TOUCH_RATE * prior;
        const double den = num + (SEPARATE_TOUCH_RATE * (1.0 - prior));
        return num / den;
    };

    const double expected_p1 = standard_step(INITIAL_CONFIDENCE); // ~0.586207
    const double expected_p2 = rapid_step(expected_p1);           // ~0.605483
    const double expected_p3 = standard_step(expected_p2);        // ~0.896874

    // 1. First hit: Uses standard rates
    gc.update_group();
    EXPECT_EQ(gc.hit_count(), 2);
    EXPECT_NEAR(gc.confidence(), expected_p1, 1e-5);

    // 2. Sleep 3 seconds: Under 5s debounce threshold -> Triggers rapid touch
    std::this_thread::sleep_for(3000ms);
    gc.update_group();
    EXPECT_EQ(gc.hit_count(), 3);
    EXPECT_NEAR(gc.confidence(), expected_p2, 1e-5);

    // 3. Sleep slightly over 5 seconds (5100ms): Clears debounce -> Standard rates
    std::this_thread::sleep_for(5100ms);
    gc.update_group();
    EXPECT_EQ(gc.hit_count(), 4);
    EXPECT_NEAR(gc.confidence(), expected_p3, 1e-5);
    EXPECT_NEAR(gc.confidence(), 0.896874, 1e-5);
}

} // namespace
} // namespace mine_tracker