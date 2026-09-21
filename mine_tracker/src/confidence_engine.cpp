#include "confidence_engine.hpp"

namespace mine_tracker {

    GroupConfidence::GroupConfidence(int group_id): group_id(group_id){}
    GroupConfidence::~GroupConfidence() = default;

    void GroupConfidence::update_group(){
        double p_true_positive = CONFIDENT_RATE;
        double p_false_positive = ACCIDENT_RATE;
        auto current_time =  std::chrono::system_clock::now();
 
        if (is_rapid_touch(current_time)) {
            p_true_positive = RAPID_TOUCH_RATE;
            p_false_positive = SEPARATE_TOUCH_RATE;
        }

        update_detection(p_true_positive, p_false_positive);
        _last_hit_time = current_time;
    }

    void GroupConfidence::update_detection(double p_true_positive, double p_false_positive) noexcept {
        double likelihood_mine = p_true_positive * _confidence_prc;
        double likelihood_noise = p_false_positive * (1.0 - _confidence_prc);
        double total = likelihood_mine + likelihood_noise;

        if (total > 1e-9) {
            _confidence_prc = likelihood_mine / total;
        }
        _hit_count++;
    }

    bool GroupConfidence::is_rapid_touch(std::chrono::system_clock::time_point now) const noexcept {
        if (_hit_count == 0) return false;
        return (now - _last_hit_time) < std::chrono::duration<double>(DEBOUNCE_SECONDS);
    }
} //namespace mine_tracker
