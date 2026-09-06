#include "controller.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "gps_decoder.hpp"
#include "udp_socket.hpp"

static const char* TAG = "Controller";

namespace mine_detector {

    struct Controller::Impl {
        TaskHandle_t taskHandle{nullptr};
        TouchSensor& sensor;
        Buzzer& buzzer;
        GpsNeo& gps;
        networking::UdpSocket& udp_socket;
        uint32_t pollIntervalMs;
        bool is_running{false};

        Impl(TouchSensor& sensor, 
                            Buzzer& buzzer, 
                            GpsNeo& gps,
                            networking::UdpSocket& udp_socket,
                            uint32_t pollIntervalMs) 
                : sensor(sensor), 
                buzzer(buzzer), 
                gps(gps), 
                udp_socket(udp_socket),
                pollIntervalMs(pollIntervalMs) {}
    };

    Controller::Controller(TouchSensor& sensor, 
                            Buzzer& buzzer, 
                            GpsNeo& gps,
                            const std::unique_ptr<networking::UdpSocket>& udp_socket,
                            uint32_t pollIntervalMs)
                : pImpl(std::make_unique<Impl>(sensor, buzzer, gps, *udp_socket, pollIntervalMs)) { }

    Controller::~Controller() {
        stop();
    }

    bool Controller::isRunning() {
        return pImpl->is_running;
    }

    bool Controller::start()
    {
        if (pImpl->is_running) {
            ESP_LOGW(TAG, "Controller loop is already running.");
            return true;
        }
        
        BaseType_t result = xTaskCreate(
            Controller::task_wrapper,
            "detector_task",
            3072, // stack size in words
            this, //passed parameter
            5, // task priority
            &pImpl->taskHandle
        );

        if (result == pdPASS) {
            pImpl->is_running = true;
            ESP_LOGI(TAG, "Controller task started successfully (interval: %ld ms)", pImpl->pollIntervalMs);
            return true;
        }

        ESP_LOGE(TAG, "Failed to create Controller task!");
        return false;
    }

    void Controller::task_wrapper(void *arg){
        auto* controller = static_cast<Controller*>(arg);
        controller->runLoop();
    }

    void  Controller::runLoop(){  
        TickType_t lastWakeTime = xTaskGetTickCount();    
        
        while (true) {
            auto gps_data = pImpl->gps.get_data();
            if (gps_data) { 
                ESP_LOGI(TAG, "[GPS FIX][%s] Lat: %.6f, Lon: %.6f, Sats: %u, Alt: %.1f m", 
                        to_string(gps_data->gp_type),
                        gps_data->latitude, 
                        gps_data->longitude, 
                        static_cast<unsigned int>(gps_data->satellite_count), 
                        gps_data->altitude);
            }

            bool touched = pImpl->sensor.isTouched();
            if (touched) {
                    ESP_LOGW(TAG, "[ALERT] Mine detected!");
                    pImpl->buzzer.turnOn();
                    pImpl->udp_socket.sendTouched();
            } else {
                    pImpl->buzzer.turnOff();
            }

            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(pImpl->pollIntervalMs));
        }

        vTaskDelete(NULL);
    }

    void Controller::stop() {
        pImpl->buzzer.turnOff();
        if (pImpl->taskHandle != nullptr) {
            TaskHandle_t handle = pImpl->taskHandle;
            pImpl->taskHandle = nullptr;
            vTaskDelete(handle);
        }
        pImpl->is_running = false;
        ESP_LOGI(TAG, "Controller task stopped.");
    }

} // namespace mine_detector