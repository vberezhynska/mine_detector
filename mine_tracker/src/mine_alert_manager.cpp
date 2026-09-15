#include "mine_alert_manager.hpp"

#include <cstdint>
#include <iostream>

#include "flags.hpp"
#include "dto/struct_library.hpp"
#include "safe_queue.hpp"

namespace mine_tracker {
    struct MineAlertManager::Impl {
        SafeQueue<mine_tracker::MineAlertData>& alert_queue;
        Flags& flags;

        explicit Impl(SafeQueue<mine_tracker::MineAlertData>& alert_queue, Flags& flags)
         : alert_queue(alert_queue), flags(flags){};
        ~Impl() = default;
    };
    
    MineAlertManager::MineAlertManager(SafeQueue<mine_tracker::MineAlertData>& alert_queue, Flags& flags) 
    : pImpl(std::make_unique<Impl>(alert_queue, flags)) {}
    MineAlertManager::~MineAlertManager() = default;

    void MineAlertManager::run(){
        MineAlertData data;
        // pop() automatically puts the thread to sleep when empty.
        // It wakes up when data arrives, OR returns false if alert_queue.stop() is called.
        while (pImpl->alert_queue.pop(data)) {
                int match_id = pImpl->flags.get_group_in_range(data.lat(), data.lon());
                if (match_id != -1){
                    std::cout << "[ MineAlertManager ] New id is in same group with " << match_id << std::endl;
                }
                
                pImpl->flags.add_flag(data.lat(), data.lon());
        }
    }
} //namespace mine_tracker 