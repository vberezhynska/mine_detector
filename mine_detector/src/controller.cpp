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

    bool Controller::startInLoop()
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
                //in next versions this one will be moved to separate thead
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

    bool Controller::start() {
        if (m_isRunning) {
            ESP_LOGW(TAG, "Controller loop is already running.");
            return true;
        }

        BaseType_t result = xTaskCreate(
            Controller::taskWrapper,
            "detector_task",
            3072,                // Stack size in words
            this,                // Parameter passed to task
            5,                   // Task priority
            &m_taskHandle
        );

        if (result == pdPASS) {
            m_isRunning = true;
            ESP_LOGI(TAG, "Controller task started successfully (interval: %ld ms)", m_pollIntervalMs);
            return true;
        }

        ESP_LOGE(TAG, "Failed to create Controller task!");
        return false;
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

    void Controller::taskWrapper(void* arg) {
        auto* controller = static_cast<Controller*>(arg);
        controller->runLoop();
    }

    void Controller::runLoop() {
        TickType_t lastWakeTime = xTaskGetTickCount();

        while (true) {
            bool touched = m_sensor.isTouched();

            if (touched) {
                ESP_LOGW(TAG, "[ALERT] Mine detected!");
                m_buzzer.turnOn();
            } else {
                m_buzzer.turnOff();
            }

            // Precise periodic delay to prevent drift
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(m_pollIntervalMs));
        }
    }
} // namespace mine_detector