#include "wifi_manager.hpp"

#include <cstring>
#include "nvs_flash.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

static const char* TAG = "wifi_manager";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

namespace networking {

    struct WifiManager::Impl {
        std::string ssid;
        std::string password;
        EventGroupHandle_t event_group{nullptr};
        int retry_num{0};
        static constexpr int MAXIMUM_RETRY = 5;

        Impl(std::string s, std::string p) 
            : ssid(std::move(s)), password(std::move(p)) {}

        ~Impl() {
            if (event_group) {
                vEventGroupDelete(event_group);
            }
        }

        static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
            auto* self = static_cast<WifiManager::Impl*>(arg);

            if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
                esp_wifi_connect();
            } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
                if (self->retry_num < MAXIMUM_RETRY) {
                    esp_wifi_connect();
                    self->retry_num++;
                    ESP_LOGI(TAG, "Retrying connection to AP (%d/%d)...", self->retry_num, MAXIMUM_RETRY);
                } else {
                    xEventGroupSetBits(self->event_group, WIFI_FAIL_BIT);
                }
            } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
                ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);
                ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
                self->retry_num = 0;
                xEventGroupSetBits(self->event_group, WIFI_CONNECTED_BIT);
            }
        }
    };

    WifiManager::WifiManager(std::string ssid, std::string password)
        : pImpl(std::make_unique<Impl>(std::move(ssid), std::move(password))) {}

    WifiManager::~WifiManager() = default;

    bool WifiManager::connect() {
        //Initialize NVS Flash internally before starting Wi-Fi stack
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
        
        pImpl->event_group = xEventGroupCreate();

        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &Impl::event_handler, pImpl.get(), &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &Impl::event_handler, pImpl.get(), &instance_got_ip));

        wifi_config_t wifi_config = {};
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), pImpl->ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), pImpl->password.c_str(), sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        EventBits_t bits = xEventGroupWaitBits(
            pImpl->event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, pdFALSE, portMAX_DELAY);

        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Connected to AP: %s", pImpl->ssid.c_str());
            return true;
        } 
        
        ESP_LOGE(TAG, "Failed to connect to AP: %s", pImpl->ssid.c_str());
        return false;
    }

} // namespace networking