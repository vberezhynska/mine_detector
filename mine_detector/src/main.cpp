#include "freertos/FreeRTOS.h"
#include "controller.hpp"
#include "touch_sensor.hpp"
#include "gps_neo.hpp"
#include "buzzer.hpp"
#include "esp_log.h"
#include "freertos/task.h"

static const char* TAG = "main";

// Pin Configuration
constexpr int TOUCH_SENSOR_PIN = 4;
constexpr int BUZZER_PIN       = 5;
constexpr int GPS_RX_PIN       = 21;
constexpr int GPS_TX_PIN       = 22;

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initializing Mine Detector application...");

    static mine_detector::TouchSensor sensor(TOUCH_SENSOR_PIN);
    static mine_detector::GpsNeo gps(GPS_RX_PIN, GPS_TX_PIN);
    static mine_detector::Buzzer buzzer( mine_detector::BuzzerType::ACTIVE, BUZZER_PIN);
    
    if (!sensor.init()) {
        ESP_LOGE(TAG, "Failed to initialize touch sensor!");
        return;
    }

    if (!buzzer.init()) {
        ESP_LOGE(TAG, "Failed to initialize buzzer!");
        return;
    }

    if (!gps.init()) {
        ESP_LOGE(TAG, "Failed to initialize GPS!");
        return;
    }

    ESP_LOGI(TAG, "Mine Detector system ready.");

    //Initialize controller
    static mine_detector::Controller controller(sensor, buzzer, gps, 1000);
    controller.start();
}