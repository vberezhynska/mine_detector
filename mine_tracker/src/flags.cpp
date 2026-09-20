#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wpsabi"
#endif

#include "flags.hpp"

#include <utility>
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

    struct Flags::Impl {
        bgi::rtree<FlagValue, bgi::rstar<16>> rtree;
        const double radius_meters;
        int next_id {0}; 
        int next_group_id {0};
        //TODO: sync with id in DB

        explicit Impl(double radius_meters) : radius_meters(radius_meters){};
        ~Impl() = default;

        int add(double lon, double lat) {
            if (lon < 0.01 || lat < 0.01) {
                DEBUG(std::format("[ERROR] Alert request has default GPS coordinates ({:.6f}, {:.6f}). GPS point will be skipped.\n", lat, lon));
                return -1;
            }

            const GeoPoint target(lon, lat);

            DEBUG(std::format(
                "[ Flags::add ] Checking location ({:.6f}, {:.6f}) within radius {}m (current rtree size: {})\n",
                lat, lon, radius_meters, rtree.size()
            ));

            auto neighbor_pair = find_closest_pair_within(target);

            if (neighbor_pair.second.id == -1) { // no neighbor_pair
                rtree.insert(std::make_pair(target, FlagMeta{next_id++, -1}));
                //TODO: store in DB?

                DEBUG(std::format(
                    "[ Flags::add ] -> No neighbor within {}m with group_id=-1 (unclustered)\n",
                    radius_meters
                ));
                return -1;
            }
            
            int assigned_group = neighbor_pair.second.group_id;

            if (assigned_group == -1) {
                assigned_group = next_group_id++; // new group is created

                LOG(std::format(
                    "[ Flags::add ] -> Found solitary neighbor flag id={} at ({:.6f}, {:.6f}). Promoting it to new group_id={}\n",
                    neighbor_pair.second.id,
                    bg::get<1>(neighbor_pair.first),
                    bg::get<0>(neighbor_pair.first),
                    assigned_group
                ));

                // Update the neighbor in R-tree from -1 to assigned_group
                rtree.remove(neighbor_pair);
                neighbor_pair.second.group_id = assigned_group;
                rtree.insert(neighbor_pair);

                LOG(std::format(
                    "[ Flags::add ] -> Updated neighbor flag id={} in R-tree to group_id={}\n",
                    neighbor_pair.second.id,
                    assigned_group
                ));
            } else {
                LOG(std::format(
                    "[ Flags::add ] -> Found existing cluster neighbor flag id={} already in group_id={}\n",
                    neighbor_pair.second.id,
                    assigned_group
                ));
            }

            int assigned_id = next_id++;
            rtree.insert(std::make_pair(target, FlagMeta{assigned_id, assigned_group}));

            LOG(std::format(
                "[ Flags::add ] -> Inserted new flag id={} into group_id={} (total rtree elements: {})\n",
                assigned_id,
                assigned_group,
                rtree.size()
            ));
                
            return assigned_group;
        }

        bool contains_within(double lon, double lat) const {
            const GeoPoint target(lon, lat);

            auto in_radius = bgi::satisfies([&](const FlagValue& val) {
                return bg::distance(val.first, target) <= radius_meters;
            });

            bool found = (rtree.qbegin(in_radius) != rtree.qend());

            DEBUG(std::format(
                "[ Flags::contains_within ] Query ({:.6f}, {:.6f}) -> {}\n",
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
                    "[ Flags::find_closest_pair_within ] No flags within {}m\n",
                    radius_meters
                ));
                return default_point;
            }

            double dist = bg::distance(it->first, target);

            if (dist <= radius_meters) {
                DEBUG(std::format(
                    "[ Flags::find_closest_pair_within ] Found closest candidate flag id={} (group={}) at distance {:.2f}m\n",
                    it->second.id,
                    it->second.group_id,
                    dist
                ));
                return *it;
            }

            DEBUG(std::format(
                "[ Flags::find_closest_pair_within ] Closest flag is at {:.2f}m (exceeds {}m radius)\n",
                dist,
                radius_meters
            ));
            return default_point;
        }
    };

    //TODO: add DB handling here
    Flags::Flags(double radius_meters) : pImpl(std::make_unique<Impl>(radius_meters)) {}
    Flags::~Flags() = default;
    Flags::Flags(Flags&&) noexcept = default;
    Flags& Flags::operator=(Flags&&) noexcept = default;

    // Public forwarding methods
    int Flags::add_flag(double lon, double lat) {
        return pImpl->add(lon, lat);
    }

    bool Flags::is_within_radius_of_any(double lon, double lat) const {
        return pImpl->contains_within(lon, lat);
    }

} // namespace mine_tracker