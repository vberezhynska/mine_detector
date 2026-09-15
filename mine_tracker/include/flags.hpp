#pragma once

#include <memory>
#include <cstdint>

namespace mine_tracker {
    struct FlagMeta {
        int id;
        int group_id;
    };

    class Flags {
    public:
        explicit Flags(double radius_meters);
        ~Flags();

        Flags(Flags&&) noexcept;
        Flags& operator=(Flags&&) noexcept;
        Flags(const Flags&) = delete;
        Flags& operator=(const Flags&) = delete;

        void add_flag(double lon, double lat);
        bool is_within_radius_of_any(double lon, double lat) const;
        int get_group_in_range(double lon, double lat) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
    };
} //namespace mine_tracker
