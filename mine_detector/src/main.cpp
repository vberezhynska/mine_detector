#include "controller.hpp"
#include "touch_sensor.hpp"
#include "buzzer.hpp"
#include "esp_log.h"
#include "freertos/task.h"

static const char* TAG = "main";

// Pin Configuration
constexpr gpio_num_t TOUCH_SENSOR_PIN = GPIO_NUM_4;
constexpr gpio_num_t BUZZER_PIN       = GPIO_NUM_5;

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initializing Mine Detector application...");

    static mine_detector::TouchSensor sensor(TOUCH_SENSOR_PIN);
    static mine_detector::Buzzer buzzer(BUZZER_PIN, mine_detector::BuzzerType::ACTIVE);
    
    if (!sensor.init()) {
        ESP_LOGE(TAG, "Failed to initialize touch sensor!");
        return;
    }

    if (!buzzer.init()) {
        ESP_LOGE(TAG, "Failed to initialize buzzer!");
        return;
    }

    ESP_LOGI(TAG, "Mine Detector system ready.");

    //Initialize controller
    static mine_detector::Controller controller(sensor, buzzer, 500);
    controller.start();
}