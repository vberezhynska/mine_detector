#include "controller.hpp"
#include "gps_decoder.hpp"
#include "esp_log.h"

static const char* TAG = "Controller";

namespace mine_detector {

    Controller::Controller(TouchSensor& sensor, Buzzer& buzzer, GpsNeo& gps, uint32_t pollIntervalMs)
        : m_sensor(sensor), m_buzzer(buzzer), m_gps(gps), m_pollIntervalMs(pollIntervalMs) {}

    Controller::~Controller() {
        stop();
    }

    bool Controller::start()
    {
        if (m_isRunning) {
            ESP_LOGW(TAG, "Controller loop is already running.");
            return true;
        }

        runInSimpleLoop();
        m_isRunning = true;
        ESP_LOGI(TAG, "Started simple loop");
        return true;
    }

    void  Controller::runInSimpleLoop(){  
            while (true) {
                //in next versions this one will be moved to separate thead + will be run with task using FreeRTOS
                // and when i start using it, parse will be used in m_gps
            auto gps_data = m_gps.get_data();
            if (gps_data) { 
                ESP_LOGI(TAG, "[GPS FIX] Lat: %.6f, Lon: %.6f, Sats: %u, Alt: %.1f m", 
                        gps_data->latitude, 
                        gps_data->longitude, 
                        static_cast<unsigned int>(gps_data->satellite_count), 
                        gps_data->altitude);
            }

            bool touched = m_sensor.isTouched();
            if (touched) {
                    ESP_LOGW(TAG, "[ALERT] Mine detected!");
                    m_buzzer.turnOn();
            } else {
                    m_buzzer.turnOff();
            }

            vTaskDelay(pdMS_TO_TICKS(m_pollIntervalMs));
        }
    }

    void Controller::stop() {
        if (m_taskHandle != nullptr) {
            vTaskDelete(m_taskHandle);
            m_taskHandle = nullptr;
        }
        m_buzzer.turnOff();
        m_isRunning = false;
        ESP_LOGI(TAG, "Controller task stopped.");
    }

} // namespace mine_detector