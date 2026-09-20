#include "mine_alert_manager.hpp"

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>

#include "flags.hpp"
#include "dto/struct_library.hpp"
#include "safe_queue.hpp"
#include "confidence_engine.hpp"
#include "mavlink_broadcaster.hpp"
#include "external/debug_macros.hpp"

namespace mine_tracker {
    struct MineAlertManager::Impl {
        SafeQueue<mine_tracker::MineAlertData>& alert_queue;
        Flags& flags;
        std::shared_ptr<mine_tracker::MavlinkBroadcaster> mavlink;
        std::unordered_map<int, GroupConfidence> groups;

        explicit Impl(
            SafeQueue<mine_tracker::MineAlertData>& alert_queue, 
            Flags& flags,
            const std::shared_ptr<mine_tracker::MavlinkBroadcaster>& mavlink)
         : alert_queue(alert_queue), flags(flags), mavlink(mavlink) {}
        ~Impl() = default;

        void send_mavlink_alert(int32_t lat_e7, int32_t lon_e7, const GroupConfidence& group_confidence) {
            if (mavlink) {
                mavlink->send_danger_zone(lat_e7, lon_e7, group_confidence);
                DEBUG("[INFO] MavLink send mine point.\n");
                return;
            }

            LOG("[ERROR] MavLink ptr is null.");
        }
    };
    
    MineAlertManager::MineAlertManager(
            SafeQueue<mine_tracker::MineAlertData>& alert_queue, 
            Flags& flags, 
            const std::shared_ptr<mine_tracker::MavlinkBroadcaster>& mavlink) 
        : pImpl(std::make_unique<Impl>(alert_queue, flags, mavlink)) {}
    MineAlertManager::~MineAlertManager() = default;

    void MineAlertManager::run() {
        MineAlertData data;
        while (pImpl->alert_queue.pop(data)) {
            DEBUG(std::format(
                "[ MineAlertManager ] Processing ping at ({:.6f}, {:.6f})\n",
                data.lat(), data.lon()
            ));

            int group_id = pImpl->flags.add_flag(data.lon(), data.lat());
            if (group_id == -1) {
                DEBUG("[ MineAlertManager ] Flag added with -1 group. No cluster created.\n");
                continue;
            }

            auto [it, inserted] = pImpl->groups.try_emplace(group_id, group_id);
            if (inserted) {
                DEBUG(std::format(
                    "[ MineAlertManager ] -> PROMOTED! Second touch confirmed nearby. Created Group {} (Baseline prior: 20%)\n",
                    group_id
                ));
            } else {
                DEBUG(std::format(
                    "[ MineAlertManager ] -> Corroborating hit for existing Group {}\n",
                    group_id
                ));
            }

            it->second.update_group();

            DEBUG(std::format(
                "[ MineAlertManager ] Group {} Status | Hits: {} | Confidence: {:.1f}%\n",
                group_id, it->second.hit_count(), it->second.confidence() * 100.0
            ));
            
            if (it->second.confidence() >= 0.80) {
                LOG(std::format(
                    "[ ALERT TRIGGERED ] *** MINE CONFIRMED in Group {} (Confidence: {:.1f}%) ***",
                    group_id, it->second.confidence() * 100.0
                ));
                
                pImpl->send_mavlink_alert(data.lat_int, data.lon_int, it->second);
            }
        }
    }
} // namespace mine_tracker