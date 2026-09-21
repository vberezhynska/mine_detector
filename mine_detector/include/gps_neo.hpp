#pragma once

#include <cstdint>
#include <optional>
#include <memory>

#include "IDevice.hpp"

//GPS NEO-6M (in ESP32 RX GPIO 21, TX GPIO 22) //leave default 16/17, but I do not have those on my ESP
//$GPRMC,123519.00,A,4807.03812,N,01131.00012,E,0.224,,230826,,,A*6C
//$GPGGA,123519.00,4807.03812,N,01131.00012,E,1,08,0.9,545.4,M,46.9,M,,*42
namespace mine_detector {
    struct GpsSystemFixData;

    class GpsNeo : IDevice {
        public: 
        explicit GpsNeo(int rx_pin = 16, int tx_pin = 17, int uart_nr = 1);
        ~GpsNeo() override;

        GpsNeo(const GpsNeo&) = delete;
        GpsNeo& operator=(const GpsNeo&) = delete;
        GpsNeo(GpsNeo&&) noexcept;
        GpsNeo& operator=(GpsNeo&&) noexcept;

        bool init() override;
        std::optional<GpsSystemFixData> get_data();
        GpsSystemFixData parse_data_gpgga(int bytes_read);

        private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;

        static constexpr size_t GPS_BUFFER_SIZE = 256;
        uint8_t m_gpsBuffer[GPS_BUFFER_SIZE]{0};

        bool m_initialized{false};
    };

} //namespace mine_detector