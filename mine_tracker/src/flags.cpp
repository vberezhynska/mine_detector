#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wpsabi"
#endif

#include "flags.hpp"

#include <utility>
#include <unordered_map>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <format>
#include <string>

#include "external/debug_macros.hpp"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace mine_tracker {
    using GeoPoint = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;
    using FlagValue = std::pair<GeoPoint, FlagMeta>;

    struct GroupRecord {
        int group_id;
        double lon;
        double lat;
        int hit_count{0};

        void add_point(double p_lon, double p_lat) {
            lon = (lon * hit_count + p_lon) / (hit_count + 1);
            lat = (lat * hit_count + p_lat) / (hit_count + 1);
            hit_count++;
        }
    };

    struct Flags::Impl {
        bgi::rtree<FlagValue, bgi::rstar<16>> rtree;
        std::unordered_map<int, GroupRecord> groups; // Tracks centroid and hits per group
        const double radius_meters;
        int next_id {0}; 
        int next_group_id {0};

        explicit Impl(double radius_meters) : radius_meters(radius_meters) {}
        ~Impl() = default;

        int add(double lon, double lat) {
            if (lon < 0.01 || lat < 0.01) {
                DEBUG(std::format("[ERROR] Alert request has default GPS coordinates ({:.6f}, {:.6f}). GPS point will be skipped.", lat, lon));
                return -1;
            }

            const GeoPoint target(lon, lat);
            auto neighbor_pair = find_closest_pair_within(target);

            // Case: No neighbor or cluster within radius -> Store as unclustered solitary flag
            if (neighbor_pair.second.id == -1) {
                int assigned_id = next_id++;
                rtree.insert(std::make_pair(target, FlagMeta{assigned_id, -1}));
                DEBUG(std::format(
                    "[ Flags::add ] -> No neighbor within {}m. Inserted solitary candidate id={}",
                    radius_meters, assigned_id
                ));
                return -1;
            }
            
            int assigned_group = neighbor_pair.second.group_id;

            //Case: Create group with first point and incert centroid into the R-tree
            if (assigned_group == -1) {
                assigned_group = next_group_id++;

                LOG(std::format(
                    "[ Flags::add ] -> Found solitary neighbor flag id={} at ({:.6f}, {:.6f}). Promoting to group_id={}",
                    neighbor_pair.second.id,
                    bg::get<1>(neighbor_pair.first),
                    bg::get<0>(neighbor_pair.first),
                    assigned_group
                ));

                rtree.remove(neighbor_pair);

                GroupRecord gr{assigned_group, bg::get<0>(neighbor_pair.first), bg::get<1>(neighbor_pair.first), 1};
                gr.add_point(lon, lat);
                groups[assigned_group] = gr;

                GeoPoint centroid(gr.lon, gr.lat);
                rtree.insert(std::make_pair(centroid, FlagMeta{assigned_group, assigned_group}));

                LOG(std::format(
                    "[ Flags::add ] -> Created Group {} centered at ({:.6f}, {:.6f}) with 2 hits",
                    assigned_group, gr.lat, gr.lon
                ));
            } 
            // Case: Matched an existing group centroid -> Corroborate hit & update centroid
            else {
                LOG(std::format(
                    "[ Flags::add ] -> Matched existing group_id={}. Updating centroid and hits.",
                    assigned_group
                ));
                // Re-insert updated centroid into R-tree
                auto& gr = groups[assigned_group];
                rtree.remove(neighbor_pair);
                gr.add_point(lon, lat);
                GeoPoint new_centroid(gr.lon, gr.lat);
                rtree.insert(std::make_pair(new_centroid, FlagMeta{assigned_group, assigned_group}));

                LOG(std::format(
                    "[ Flags::add ] -> Group {} centroid shifted to ({:.6f}, {:.6f}) (Total hits: {})",
                    assigned_group, gr.lat, gr.lon, gr.hit_count
                ));
            }

            return assigned_group;
        }

        bool contains_within(double lon, double lat) const {
            const GeoPoint target(lon, lat);

            auto in_radius = bgi::satisfies([&](const FlagValue& val) {
                return bg::distance(val.first, target) <= radius_meters;
            });

            bool found = (rtree.qbegin(in_radius) != rtree.qend());

            DEBUG(std::format(
                "[ Flags::contains_within ] Query ({:.6f}, {:.6f}) -> {}",
                lat, lon, (found ? "YES" : "NO")
            ));
            return found;
        }

        std::pair<GeoPoint, FlagMeta> find_closest_pair_within(const GeoPoint& target) const {
            const std::pair<GeoPoint, FlagMeta> default_point{GeoPoint(0.0, 0.0), FlagMeta{-1, -1}};
            auto query = bgi::nearest(target, 1);
            auto it = rtree.qbegin(query);

            if (it == rtree.qend()) {
                DEBUG(std::format(
                    "[ Flags::find_closest_pair_within ] No flags within {}m",
                    radius_meters
                ));
                return default_point;
            }

            double dist = bg::distance(it->first, target);

            if (dist <= radius_meters) {
                DEBUG(std::format(
                    "[ Flags::find_closest_pair_within ] Found closest entry id={} (group={}) at distance {:.2f}m",
                    it->second.id,
                    it->second.group_id,
                    dist
                ));
                return *it;
            }

            DEBUG(std::format(
                "[ Flags::find_closest_pair_within ] Closest entry is at {:.2f}m (exceeds {}m radius)",
                dist,
                radius_meters
            ));
            return default_point;
        }
    };

    Flags::Flags(double radius_meters) : pImpl(std::make_unique<Impl>(radius_meters)) {}
    Flags::~Flags() = default;
    Flags::Flags(Flags&&) noexcept = default;
    Flags& Flags::operator=(Flags&&) noexcept = default;

    int Flags::add_flag(double lon, double lat) {
        return pImpl->add(lon, lat);
    }

    bool Flags::is_within_radius_of_any(double lon, double lat) const {
        return pImpl->contains_within(lon, lat);
    }

} // namespace mine_tracker