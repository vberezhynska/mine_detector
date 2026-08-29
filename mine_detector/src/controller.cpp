#include "controller.hpp"
#include "esp_log.h"

static const char* TAG = "Controller";

namespace mine_detector {

    Controller::Controller(TouchSensor& sensor, Buzzer& buzzer, uint32_t pollIntervalMs)
        : m_sensor(sensor), m_buzzer(buzzer), m_pollIntervalMs(pollIntervalMs) {}

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
            bool touched = m_sensor.isTouched();

        if (touched) {
                ESP_LOGW(TAG, "[ALERT] Mine detected!");
                m_buzzer.turnOn();
        } else {
                ESP_LOGI(TAG, "[IDLE] Area clear.");
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
                ESP_LOGI(TAG, "[IDLE] Area clear.");
                m_buzzer.turnOff();
            }

            // Precise periodic delay to prevent drift
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(m_pollIntervalMs));
        }
    }

} // namespace mine_detector