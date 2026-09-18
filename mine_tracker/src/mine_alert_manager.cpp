#include "mine_alert_manager.hpp"

#include <cstdint>
#include <iostream>
#include <unordered_map>

#include "flags.hpp"
#include "dto/struct_library.hpp"
#include "safe_queue.hpp"
#include "confidence_engine.hpp"

namespace mine_tracker {
    struct MineAlertManager::Impl {
        SafeQueue<mine_tracker::MineAlertData>& alert_queue;
        Flags& flags;
        std::unordered_map<int, GroupConfidence> groups;

        explicit Impl(SafeQueue<mine_tracker::MineAlertData>& alert_queue, Flags& flags)
         : alert_queue(alert_queue), flags(flags){};
        ~Impl() = default;
    };
    
    MineAlertManager::MineAlertManager(SafeQueue<mine_tracker::MineAlertData>& alert_queue, Flags& flags) 
        : pImpl(std::make_unique<Impl>(alert_queue, flags)) {}
    MineAlertManager::~MineAlertManager() = default;

    void MineAlertManager::run(){
        MineAlertData data;
        while (pImpl->alert_queue.pop(data)) {
            //TODO: Change to LOG [DEBUG]
                std::cout << "[ MineAlertManager ] Processing ping at (" 
                    << std::fixed << std::setprecision(6) 
                    << data.lat() << ", " << data.lon() << ")\n";

                int group_id = pImpl->flags.add_flag(data.lon(), data.lat());
                if (group_id == -1) {
                    std::cout << "[ MineAlertManager ] Flag added with -1 group. No claster created " << std::endl;
                    continue;
                }

                auto [it, inserted] = pImpl->groups.try_emplace(group_id, group_id);
                //TODO: Change to LOG [DEBUG]
                if (inserted) {
                    std::cout << "[ MineAlertManager ] -> PROMOTED! Second touch confirmed nearby. Created Group " 
                        << group_id << " (Baseline prior: 20%)\n";
                } else {
                    std::cout << "[ MineAlertManager ] -> Corroborating hit for existing Group " << group_id << "\n";
                }

                it->second.update_group();

                 //TODO: Change to LOG [DEBUG]
                std::cout << "[ MineAlertManager ] Group " << group_id 
                  << " Status | Hits: " << it->second.hit_count()
                  << " | Confidence: " << std::fixed << std::setprecision(1) 
                  << (it->second.confidence() * 100.0) << "%\n";
                
                //TODO: Change to LOG [DEBUG]
                if (it->second.confidence() >= 0.80) {
                    std::cout << "[ ALERT TRIGGERED ] *** MINE CONFIRMED in Group " << group_id 
                            << " (Confidence: " << (it->second.confidence() * 100.0) << "%) ***\n";
                    // TODO: send_qgc_danger_circle(...)
                }
                // TODO: think about first touch. How to make sure that it's "Clear one". Add time to triggering it?
        }
    }
} //namespace mine_tracker 