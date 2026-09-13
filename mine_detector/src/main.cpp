#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <string>
#include <memory>

#include "controller.hpp"
#include "touch_sensor.hpp"
#include "gps_neo.hpp"
#include "buzzer.hpp"
#include "udp_socket.hpp"
#include "wifi_manager.hpp"
#include "http_client.hpp"

static const char* TAG = "main";

// Hardware Pin Configuration
constexpr int TOUCH_SENSOR_PIN = 4;
constexpr int BUZZER_PIN       = 5;
constexpr int GPS_RX_PIN       = 21;
constexpr int GPS_TX_PIN       = 22;

// Network Configuration
constexpr uint16_t UDP_PORT        = 5005;
constexpr const char* UDP_IP       = "10.42.0.1";
//constexpr const char* UDP_IP       = "192.168.0.199";
constexpr const char* WIFI_SSID     = "ESP32_Pi_Network";
constexpr const char* WIFI_PASS     = "PiSecretKey123";

//constexpr const char* HTTP_BASE_URL     = "192.168.0.199";
//constexpr const char* HTTP_BASE_URL     = "http://10.42.0.1";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initializing Mine Detector application...");

    // 1. Instantiate Hardware Peripheral Drivers (Static for lifetime management)
    static mine_detector::TouchSensor sensor(TOUCH_SENSOR_PIN);
    static mine_detector::GpsNeo gps(GPS_RX_PIN, GPS_TX_PIN);
    static mine_detector::Buzzer buzzer(mine_detector::BuzzerType::ACTIVE, BUZZER_PIN);

    // WiFi
    auto wifi = networking::WifiManager(WIFI_SSID, WIFI_PASS);
    if (!wifi.connect()) {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi AP.");
        return;
    }

    if (!sensor.init()) {
        ESP_LOGE(TAG, "Failed to initialize touch sensor.");
        return;
    }

    if (!buzzer.init()) {
        ESP_LOGE(TAG, "Failed to initialize buzzer.");
        return;
    }

    if (!gps.init()) {
        ESP_LOGE(TAG, "Failed to initialize GPS.");
        return;
    }

    // 5. Create and Initialize UDP Socket Descriptor (Wi-Fi is now active)
    static auto udp_socket = std::make_unique<networking::UdpSocket>(UDP_IP, UDP_PORT);

    if (!udp_socket->init()) {
        ESP_LOGE(TAG, "Failed to initialize UDP socket.");
        return;
    }

    static auto http_client = std::make_unique<networking::HttpClient>(UDP_IP);
    if (!http_client->init()){
        ESP_LOGE(TAG, "Failed to initialize HTTP client.");
        return;
    }

    ESP_LOGI(TAG, "Mine Detector system successfully initialized.");

    static mine_detector::Controller controller(sensor, buzzer, gps, udp_socket, http_client, 1000);
    xTaskCreate(
        [](void* arg) {
            auto* ctrl = static_cast<mine_detector::Controller*>(arg);
            ctrl->start();
            vTaskDelete(NULL);
        },
        "controller_task",
        4096,         // Stack size in bytes
        &controller,  // Task input parameter
        5,            // Task priority
        NULL
    ); // ESP32 main task exits or sleeps while thread runs
}