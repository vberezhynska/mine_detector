#pragma once

#include <chrono>

namespace mine_tracker {
    //
    //Expected 3 clear touches with a 5 sec time delay between all of them
    //
    static constexpr double DEBOUNCE_SECONDS = 5.0; // ignore rapid switch bounce
    static constexpr double INITIAL_CONFIDENCE = 0.20; // 5% baseline prior
    static constexpr double CONFIDENT_RATE = 0.85; // P(Touch | Mine)
    static constexpr double ACCIDENT_RATE = 0.15; // P(Touch | Clear)
    static constexpr double RAPID_TOUCH_RATE = 0.52; // touch that occurred too quickly < 5 sec. probabaly related to previous "alarm"
    static constexpr double SEPARATE_TOUCH_RATE = 0.48; // probabaly separate touch

    class GroupConfidence {
        public:
            const int group_id;    
            [[nodiscard]] double confidence() const noexcept { return _confidence_prc; }
            [[nodiscard]] int hit_count() const noexcept { return _hit_count; }
            
            explicit GroupConfidence(int group_id);
            ~GroupConfidence();
        void update_group();

        private:
            int _hit_count {1}; //first hit is added to the geo map with group -1
            double _confidence_prc {INITIAL_CONFIDENCE};
            std::chrono::system_clock::time_point _last_hit_time;

            void update_detection(double p_true_positive, double p_false_positive) noexcept;
            bool is_rapid_touch(std::chrono::system_clock::time_point now) const noexcept;
    };
} //namespace mine_tracker 