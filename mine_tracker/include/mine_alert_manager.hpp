#pragma once

#include <memory>
#include "safe_queue.hpp"


namespace mine_tracker {
    class Flags;
    class MavlinkBroadcaster;
    struct MineAlertData;

    class MineAlertManager {
        public:
        explicit MineAlertManager(
            mine_tracker::SafeQueue<mine_tracker::MineAlertData>& alert_queue, 
            Flags& flags,
            const std::shared_ptr<mine_tracker::MavlinkBroadcaster>& mavlink);
        ~MineAlertManager();

        MineAlertManager(MineAlertManager&&) noexcept;
        MineAlertManager& operator=(MineAlertManager&&) noexcept;
        MineAlertManager(const MineAlertManager&) = delete;
        MineAlertManager& operator=(const MineAlertManager&) = delete;
        
        void run();

        private:
            struct Impl;
            std::unique_ptr<Impl> pImpl;        
    };
} //namespace mine-tracker