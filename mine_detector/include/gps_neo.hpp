#pragma once

#include <cstdint>
#include <optional>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "IDevice.hpp"
#include "soc/gpio_num.h"



//GPS NEO-6M (in ESP32 RX GPIO 21, TX GPIO 22) //leave default 16/17, but I do not have those on my ESP
//$GPRMC,123519.00,A,4807.03812,N,01131.00012,E,0.224,,230826,,,A*6C
//$GPGGA,123519.00,4807.03812,N,01131.00012,E,1,08,0.9,545.4,M,46.9,M,,*42
namespace mine_detector {
    struct GpsSystemFixData;

    class GpsNeo : IDevice {
        public: 
        explicit GpsNeo(gpio_num_t rx_pin = GPIO_NUM_16, gpio_num_t tx_pin = GPIO_NUM_17, uart_port_t uart_nr = UART_NUM_1);
        ~GpsNeo() override;

        bool init() override;
        std::optional<GpsSystemFixData> get_data();
        GpsSystemFixData parse_data_gpgga(int bytes_read);

        private:
        gpio_num_t m_rx_pin; //recive
        gpio_num_t m_tx_pin; //transmit
        uart_port_t m_uart_nr;

        static constexpr size_t GPS_BUFFER_SIZE = 256;
        uint8_t m_gpsBuffer[GPS_BUFFER_SIZE]{0};

        bool m_initialized{false};
    };

} //namespace mine_detector