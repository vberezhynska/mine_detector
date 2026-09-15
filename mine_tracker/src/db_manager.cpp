#include "db_manager.hpp"

#include <sqlite3.h>
#include <stdint.h>
#include <mutex>
#include <stdexcept>

namespace data {
   struct DbManager::Impl {
        sqlite3* db = nullptr;
        std::mutex mutex;

        explicit Impl(const std::string& db_path) {
            if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
                std::string err_msg = db ? sqlite3_errmsg(db) : "Unknown error";
                throw std::runtime_error("Failed to open DB: " + err_msg);
            }
            init_schema();
        }

        ~Impl() {
            if (db) {
                sqlite3_close(db);
            }
        }

        void init_schema() {
            // WAL mode reduces write contention and SD card wear on Raspberry Pi
            sqlite3_exec(db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);

            const char* schema = R"(
                CREATE TABLE IF NOT EXISTS detections (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    latitude REAL NOT NULL,
                    longitude REAL NOT NULL,
                    mine_group_id INTEGER NOT NULL,
                    detected_at DATETIME DEFAULT CURRENT_TIMESTAMP
                );
            )";

            char* err_msg = nullptr;
            if (sqlite3_exec(db, schema, nullptr, nullptr, &err_msg) != SQLITE_OK) {
                std::string err = err_msg;
                sqlite3_free(err_msg);
                throw std::runtime_error("Schema init failed: " + err);
            }
        }

        void insert(double lat, double lon, int16_t groupId) {
            std::lock_guard<std::mutex> lock(mutex);

            const char* sql = "INSERT INTO detections (latitude, longitude, strength) VALUES (?, ?, ?);";
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
                return;
            }

            sqlite3_bind_double(stmt, 1, lat);
            sqlite3_bind_double(stmt, 2, lon);
            sqlite3_bind_double(stmt, 3, groupId);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        std::vector<MineRecord> get_all() {
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<MineRecord> records;

            const char* sql = "SELECT id, latitude, longitude, groupId, detected_at FROM detections;";
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    MineRecord r;
                    r.id = sqlite3_column_int(stmt, 0);
                    r.latitude = sqlite3_column_double(stmt, 1);
                    r.longitude = sqlite3_column_double(stmt, 2);
                    r.groupId = sqlite3_column_double(stmt, 3);
                    
                    const unsigned char* ts = sqlite3_column_text(stmt, 4);
                    r.timestamp = ts ? reinterpret_cast<const char*>(ts) : "";

                    records.push_back(std::move(r));
                }
            }
            sqlite3_finalize(stmt);
            return records;
        }
    };

        DbManager::DbManager(const std::string& db_path)
            : pImpl(std::make_unique<Impl>(db_path)) {}
        DbManager::~DbManager() = default;

        DbManager::DbManager(DbManager&&) noexcept = default;
        DbManager& DbManager::operator=(DbManager&&) noexcept = default;

        void DbManager::insert_detection(double lat, double lon, int16_t groupId) {
            pImpl->insert(lat, lon, groupId);
        }

        std::vector<MineRecord> DbManager::get_all_records() {
            return pImpl->get_all();
    }
} //database