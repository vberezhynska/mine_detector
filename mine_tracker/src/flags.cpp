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

        void add(double lon, double lat) {
            const GeoPoint target(lon, lat);

            // Find if it falls within the radius of an existing flag
            auto point_in_radius = find_first_meta_within(target);
            
            int assigned_group = (point_in_radius.group_id != -1) ? point_in_radius.group_id : next_group_id++;
            int assigned_id = next_id++;

            rtree.insert(std::make_pair(target, FlagMeta{assigned_id, assigned_group}));
                //TODO: store in DB?
        }

        bool contains_within(double lon, double lat) const {
            const GeoPoint target(lon, lat);

            auto in_radius = bgi::satisfies([&](const FlagValue& val) {
                return bg::distance(val.first, target) <= radius_meters;
            });

            return rtree.qbegin(in_radius) != rtree.qend();
        }

        FlagMeta find_first_meta_within(const GeoPoint& target) const {
            auto in_radius = bgi::satisfies([&](const FlagValue& val) {
                return bg::distance(val.first, target) <= radius_meters;
            });

            auto it = rtree.qbegin(in_radius);
            if (it != rtree.qend()) {
                return it->second;
            }
            return FlagMeta{-1, -1};
        }
    };

    //TODO: add DB handling here
    Flags::Flags(double radius_meters) : pImpl(std::make_unique<Impl>(radius_meters)) {}
    Flags::~Flags() = default;
    Flags::Flags(Flags&&) noexcept = default;
    Flags& Flags::operator=(Flags&&) noexcept = default;

    // Public forwarding methods
    void Flags::add_flag(double lon, double lat) {
        pImpl->add(lon, lat);
    }

    bool Flags::is_within_radius_of_any(double lon, double lat) const {
        return pImpl->contains_within(lon, lat);
    }

    int Flags::get_group_in_range(double lon, double lat) const {
        return (pImpl->find_first_meta_within(GeoPoint(lon, lat))).group_id; //TODO: Check how that works when no flags are in queue
    }
} //namespace mine_tracker