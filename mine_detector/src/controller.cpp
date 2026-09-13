#include "controller.hpp"
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "gps_decoder.hpp"
#include "udp_socket.hpp"
#include "http_client.hpp"
#include "external/telemetry_types.hpp"

static const char* TAG = "Controller";

namespace mine_detector {

    struct Controller::Impl {
        TaskHandle_t taskHandle{nullptr};
        TouchSensor& sensor;
        Buzzer& buzzer;
        GpsNeo& gps;
        networking::UdpSocket& udp_socket;
        networking::HttpClient& http_client;
        uint32_t pollIntervalMs;
        bool is_running{false};
        GpsSystemFixData last_gps_data{ 
            .gp_type = mine_tracker::NMEA_Type::UNKNOWN,
            .latitude = 0.0f, 
            .longitude = 0.0f };

        Impl(TouchSensor& sensor, 
                            Buzzer& buzzer, 
                            GpsNeo& gps,
                            networking::UdpSocket& udp_socket,
                            networking::HttpClient& http_client,
                            uint32_t pollIntervalMs) 
                : sensor(sensor), 
                buzzer(buzzer), 
                gps(gps), 
                udp_socket(udp_socket),
                http_client(http_client),
                pollIntervalMs(pollIntervalMs) {}
    };

    Controller::Controller(TouchSensor& sensor, 
                            Buzzer& buzzer, 
                            GpsNeo& gps,
                            const std::unique_ptr<networking::UdpSocket>& udp_socket,
                            const std::unique_ptr<networking::HttpClient>& http_client,
                            uint32_t pollIntervalMs)
                : pImpl(std::make_unique<Impl>(sensor, buzzer, gps, *udp_socket, *http_client, pollIntervalMs)) { }

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
                        mine_tracker::to_string(gps_data->gp_type),
                        gps_data->latitude, 
                        gps_data->longitude, 
                        static_cast<unsigned int>(gps_data->satellite_count), 
                        gps_data->altitude);

                pImpl->udp_socket.sendCoordinates(*gps_data);
                pImpl->last_gps_data = *gps_data;
            }

            bool touched = pImpl->sensor.isTouched();
            if (touched) {
                    ESP_LOGW(TAG, "[ALERT] Mine detected!");
                    
                    pImpl->http_client.sendMineAlert(
                        pImpl->last_gps_data.latitude_int, 
                        pImpl->last_gps_data.longitude_int,
                        static_cast<uint8_t>(pImpl->last_gps_data.gp_type));

                    pImpl->buzzer.turnOn();
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