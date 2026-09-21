#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include <memory>
#include <string>

#include "buzzer.hpp"
#include "controller.hpp"
#include "gps_neo.hpp"
#include "http_client.hpp"
#include "touch_sensor.hpp"
#include "udp_socket.hpp"
#include "wifi_manager.hpp"

static const char* TAG = "main";

// Hardware Pin Configuration
constexpr int TOUCH_SENSOR_PIN = 4;
constexpr int BUZZER_PIN       = 5;
constexpr int GPS_RX_PIN       = 21;
constexpr int GPS_TX_PIN       = 22;

// Network Configuration
constexpr uint16_t UDP_PORT    = 5005;
constexpr uint16_t HTTP_PORT   = 8080;
constexpr const char* UDP_IP   = "10.42.0.1";
constexpr const char* WIFI_SSID = "ESP32_Pi_Network";
constexpr const char* WIFI_PASS = "PiSecretKey123";

constexpr const char* HTTP_BASE_URL = "http://10.42.0.1";

// Helper for unrecoverable initialization failures
[[noreturn]] static void handle_init_failure(const char* message) {
    ESP_LOGE(TAG, "%s. Restarting system in 5 seconds...", message);
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_restart();
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initializing Mine Detector application...");

    // 1. Initialize NVS (required for Wi-Fi storage in ESP-IDF)
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);

    // 2. Hardware Peripheral Drivers (Static storage ensures driver lifetimes persist)
    static mine_detector::TouchSensor sensor(TOUCH_SENSOR_PIN);
    static mine_detector::GpsNeo gps(GPS_RX_PIN, GPS_TX_PIN);
    static mine_detector::Buzzer buzzer(mine_detector::BuzzerType::ACTIVE, BUZZER_PIN);

    // 3. Wi-Fi Manager (Must be static to avoid destruction on app_main task exit)
    static networking::WifiManager wifi(WIFI_SSID, WIFI_PASS);
    if (!wifi.connect()) {
        handle_init_failure("Failed to connect to Wi-Fi AP");
    }

    // 4. Initialize Peripherals
    if (!sensor.init()) {
        handle_init_failure("Failed to initialize touch sensor");
    }

    if (!buzzer.init()) {
        handle_init_failure("Failed to initialize buzzer");
    }

    if (!gps.init()) {
        handle_init_failure("Failed to initialize GPS");
    }

    // 5. Network Clients (Wi-Fi is now active)
    static auto udp_socket = std::make_unique<networking::UdpSocket>(UDP_IP, UDP_PORT);
    if (!udp_socket->init()) {
        handle_init_failure("Failed to initialize UDP socket");
    }

    static auto http_client = std::make_unique<networking::HttpClient>(HTTP_BASE_URL, HTTP_PORT);
    if (!http_client->init()) {
        handle_init_failure("Failed to initialize HTTP client");
    }

    ESP_LOGI(TAG, "Mine Detector system successfully initialized.");

    // 6. Create Controller and Spawn Task
    static mine_detector::Controller controller(sensor, buzzer, gps, udp_socket, http_client, 1000);

    BaseType_t task_created = xTaskCreate(
        [](void* arg) {
            auto* ctrl = static_cast<mine_detector::Controller*>(arg);
            ctrl->start();
            vTaskDelete(NULL);
        },
        "controller_task",
        4096,          // Stack size in bytes
        &controller,   // Task input parameter
        5,             // Task priority
        NULL
    );

    if (task_created != pdPASS) {
        handle_init_failure("Failed to create controller_task");
    }
}