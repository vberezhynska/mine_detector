#include <optional>

#include "gps_neo.hpp"
#include "gps_decoder.hpp"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/uart_types.h"

static const char* TAG = "GPS_NEO";

namespace mine_detector {

    GpsNeo::GpsNeo(gpio_num_t rx_pin, gpio_num_t tx_pin, uart_port_t uart_nr) : m_rx_pin(rx_pin), m_tx_pin(tx_pin), m_uart_nr(uart_nr){}
    GpsNeo::~GpsNeo() {
        if (m_initialized){
            uart_driver_delete(m_uart_nr);
        }
    }
    
    bool GpsNeo::init(){
        uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_DEFAULT
        };

        //set config parameters
        esp_err_t error = uart_param_config(m_uart_nr, &uart_config);
        if (error != ESP_OK){
            ESP_LOGE(TAG, "Failed to configure UART parameters");
            return false;
        }

        //set UART pins
        error = uart_set_pin(m_uart_nr, m_tx_pin, m_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (error != ESP_OK){
            ESP_LOGE(TAG, "Failed to set UART pins");
            return false;
        }

        //set UART RX buffer. No TX, no event queue
        error = uart_driver_install(m_uart_nr, 1024, 0, 0, NULL, 0);
        if (error != ESP_OK){
            ESP_LOGE(TAG, "Failed to install UART driver");
            return false;
        }

        uart_flush_input(m_uart_nr); //start with slean buffer
        m_initialized = true;
        ESP_LOGI(TAG, "GPS NEO-6M UART initialized successfully (RX: GPIO%d, TX: GPIO%d)", m_rx_pin, m_tx_pin);
        return true;
    }
    

    std::optional<GpsSystemFixData> GpsNeo::get_data(){

        if (!m_initialized){
            ESP_LOGE(TAG, "Not initialized");
            return std::nullopt;
        }

        auto bytes_read = uart_read_bytes(m_uart_nr, m_gpsBuffer, GPS_BUFFER_SIZE - 1, pdMS_TO_TICKS(1000));
        if (bytes_read < 0)
        {
            ESP_LOGE(TAG, "No data recieved.");
            return std::nullopt;
        }

        m_gpsBuffer[bytes_read] = '\0'; // Safety null-termination
        auto raw_data = reinterpret_cast<char*>(m_gpsBuffer);
        GpsSystemFixData data{};
        auto isParsed = GgaDecoder().parse(raw_data, data);
        if(isParsed){
            return data;
        }

        ESP_LOGE(TAG, "Data were not parsed");
        return std::nullopt;
    }
} //namespace mine_detector