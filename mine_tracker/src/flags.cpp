#include "flags.hpp"

#include <utility>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace mine_tracker {
    using GeoPoint = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;
    using FlagValue = std::pair<GeoPoint, FlagMeta>;

    
    struct Flags::Impl {
        bgi::rtree<FlagValue, bgi::rstar<16>> rtree;
        const double radius_meters;
        int next_id {0}; 
        int next_group_id {0}; //TODO: sync with id in DB

        explicit Impl(double radius_meters) : radius_meters(radius_meters){};
        ~Impl() = default;

        int add(double lon, double lat) {
            const GeoPoint target(lon, lat);
            //TODO: Change to LOG [DEBUG]
            std::cout << "[ Flags::add ] Checking location (" 
                      << std::fixed << std::setprecision(6) << lat << ", " << lon 
                      << ") within radius " << radius_meters << "m (current rtree size: " 
                      << rtree.size() << ")\n";

            auto neighbor_pair = find_closest_pair_within(target);

            if (neighbor_pair.second.id == -1) { //no neighbor_pair
                rtree.insert(std::make_pair(target, FlagMeta{next_id++, -1}));
                //TODO: store in DB?
                //TODO: Change to LOG [DEBUG]
                std::cout << "[ Flags::add ] -> No neighbor within " << radius_meters 
                          << " with group_id=-1 (unclustered)\n";
                return -1;
            }
            
            int assigned_group = neighbor_pair.second.group_id;

            if (assigned_group == -1){
                assigned_group = next_group_id++; //new group is created
                //TODO: Change to LOG [DEBUG]
                std::cout << "[ Flags::add ] -> Found solitary neighbor flag id=" << neighbor_pair.second.id 
                          << " at (" << bg::get<1>(neighbor_pair.first) << ", " << bg::get<0>(neighbor_pair.first)
                          << "). Promoting it to new group_id=" << assigned_group << "\n";

                // Update the neighbor in R-tree from -1 to assigned_group
                rtree.remove(neighbor_pair);
                neighbor_pair.second.group_id = assigned_group;
                rtree.insert(neighbor_pair);
                //TODO: Change to LOG [DEBUG]
                std::cout << "[ Flags::add ] -> Updated neighbor flag id=" << neighbor_pair.second.id 
                          << " in R-tree to group_id=" << assigned_group << "\n";
            } else {
                //TODO: Change to LOG [DEBUG]
                std::cout << "[ Flags::add ] -> Found existing cluster neighbor flag id=" << neighbor_pair.second.id 
                          << " already in group_id=" << assigned_group << "\n";
            }

            int assigned_id = next_id++;
            rtree.insert(std::make_pair(target, FlagMeta{assigned_id, assigned_group}));
            //TODO: Change to LOG [DEBUG]
            std::cout << "[ Flags::add ] -> Inserted new flag id=" << assigned_id 
                      << " into group_id=" << assigned_group 
                      << " (total rtree elements: " << rtree.size() << ")\n";
                
            return assigned_group;
        }

        bool contains_within(double lon, double lat) const {
            const GeoPoint target(lon, lat);

            auto in_radius = bgi::satisfies([&](const FlagValue& val) {
                return bg::distance(val.first, target) <= radius_meters;
            });

            bool found = (rtree.qbegin(in_radius) != rtree.qend());
            //TODO: Change to LOG [DEBUG]
            std::cout << "[ Flags::contains_within ] Query (" << lat << ", " << lon 
                      << ") -> " << (found ? "YES" : "NO") << "\n";
            return found;
        }
        //TODO: Update this to search my Radius or region. Not sure if it should be first point
        //TODO: but I am leaving it as it is for now
        std::pair<GeoPoint, FlagMeta> find_closest_pair_within(const GeoPoint& target) const {
            const std::pair<GeoPoint, FlagMeta> default_point{GeoPoint(0.0, 0.0), FlagMeta{-1, -1}};
            auto query = bgi::nearest(target, 1);
            auto it = rtree.qbegin(query);

            if (it == rtree.qend()){
                //TODO: Change to LOG [DEBUG]
                std::cout << "[ Flags::find_closest_pair_within ] No flags within " 
                        << radius_meters << "m\n";
                return default_point;
            }

            double dist = bg::distance(it->first, target);
            //TODO: Change to LOG [DEBUG]
            if (dist <= radius_meters) {
                std::cout << "[ Flags::find_first_pair_within ] Found closest candidate flag id=" 
                << it->second.id << " (group=" << it->second.group_id 
                << ") at distance " << std::fixed << std::setprecision(2) 
                << dist << "m\n";
                return *it;
            }
            std::cout << "[ Flags::find_closest_pair_within ] Closest flag is at " 
                << std::fixed << std::setprecision(2) << dist 
                << "m (exceeds " << radius_meters << "m radius)\n";
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

} //namespace mine_tracker