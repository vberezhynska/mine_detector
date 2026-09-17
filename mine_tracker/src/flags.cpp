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

            auto neighbor_pair = find_first_pair_within(target);
            //TODO: return opt here?
            if (neighbor_pair.second.id == -1) { //no neighbor_pair
                rtree.insert(std::make_pair(target, FlagMeta{next_id++, -1}));
                //TODO: store in DB?
                return -1;
            }
            
            int assigned_group = neighbor_pair.second.group_id;

            if (assigned_group == -1){
                assigned_group = next_group_id++; //new group is created
                
                // Update the neighbor in R-tree from -1 to assigned_group
                rtree.remove(neighbor_pair);
                neighbor_pair.second.group_id = assigned_group;
                rtree.insert(neighbor_pair);
            }

            rtree.insert(std::make_pair(target, FlagMeta{next_id++, assigned_group}));
                
            return assigned_group;
        }

        bool contains_within(double lon, double lat) const {
            const GeoPoint target(lon, lat);

            auto in_radius = bgi::satisfies([&](const FlagValue& val) {
                return bg::distance(val.first, target) <= radius_meters;
            });

            return rtree.qbegin(in_radius) != rtree.qend();
        }
        //TODO: Update this to search my Radius or region. Not sure if it should be first point
        //TODO: but I am leaving it as it is for now
        std::pair<GeoPoint, FlagMeta> find_first_pair_within(const GeoPoint& target) const {
            auto in_radius = bgi::satisfies([&](const FlagValue& val) {
                return bg::distance(val.first, target) <= radius_meters;
            });

            auto it = rtree.qbegin(in_radius);
            if (it != rtree.qend()) {
                return *it;
            }
            return {GeoPoint(0.0, 0.0), FlagMeta{-1, -1}};
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