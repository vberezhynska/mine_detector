#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace data {
    struct MineRecord {
        int id;
        double latitude;
        double longitude;
        int16_t groupId;
        std::string timestamp;
    };

    class DbManager {
    public:
        explicit DbManager(const std::string& db_path);
        ~DbManager();

        DbManager(DbManager&&) noexcept;
        DbManager& operator=(DbManager&&) noexcept;

        DbManager(const DbManager&) = delete;
        DbManager& operator=(const DbManager&) = delete;

        void insert_detection(double lon, double lat, int16_t groupId);
        std::vector<MineRecord> get_all_records();

    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
    };
}
