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
                int group_id = pImpl->flags.add_flag(data.lon(), data.lat());
                if (group_id == -1) {
                    std::cout << "[ MineAlertManager ] Flag added with -1 group. No claster created " << std::endl;
                    continue;
                }

                auto [it, inserted] = pImpl->groups.try_emplace(group_id, group_id);

                it->second.update_group();
                // TODO: think about first touch. How to make sure that it's "Clear one". Add time to triggering it?
        }
    }
} //namespace mine_tracker 