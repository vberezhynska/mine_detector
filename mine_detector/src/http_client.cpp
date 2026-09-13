#include "esp_http_client.h"

#include <external/json.hpp>
#include "http_client.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>

 using json = nlohmann::json;
 static const char* TAG = "HttpClient";

namespace networking {
    struct HttpClient::Impl{
        std::string baseUrl;
        const std::string alert_url = "/v1/api/alerts";
        uint16_t port;
        
        esp_http_client_config_t config = {};
        esp_http_client_handle_t client = {};

        Impl(std::string url, uint16_t port) : baseUrl(std::move(url)), port(port){}

        ~Impl() {
            if (client) {
                esp_http_client_cleanup(client);
                client = nullptr;
            }
        }
    };

    HttpClient::HttpClient(std::string baseUrl, uint16_t port) : pImpl(std::make_unique<Impl>(std::move(baseUrl), port)) { }

    HttpClient::~HttpClient() = default;

    bool HttpClient::init() {
        pImpl->config.url = pImpl->baseUrl.c_str();
        pImpl->config.port = pImpl->port;
        pImpl->config.timeout_ms = 5000;
        
        pImpl->client = esp_http_client_init(&pImpl->config);
        if (!pImpl->client) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client.");
            return false;
        }

        ESP_LOGI(TAG, "HTTP client initialized for target: %s (Port: %u)", 
             pImpl->baseUrl.c_str(), pImpl->port);

        return true;
    }
    //TODO: update with TelemetryPayload
    bool HttpClient::sendMineAlert(int32_t latitude, int32_t longitude, int8_t gp_type){
            json alertJson = {
                {"event", "MINE_DETECTED"},
                {"lat", latitude},
                {"lon", longitude},
                {"gpType", gp_type}
            };

        std::string payload = alertJson.dump();
        std::string fullUrl = pImpl->baseUrl + pImpl->alert_url;

        ESP_LOGI(TAG, "HTTP sendMineAlert will send to fullUrl: %s", 
             fullUrl.c_str());

        esp_http_client_set_url(pImpl->client, fullUrl.c_str());
        esp_http_client_set_method(pImpl->client, HTTP_METHOD_POST);
        esp_http_client_set_header(pImpl->client, "Content-Type", "application/json");
        esp_http_client_set_header(pImpl->client, "Connection", "close"); //close socket connection
        esp_http_client_set_post_field(pImpl->client, payload.c_str(), static_cast<int>(payload.length()));

        esp_err_t err = esp_http_client_perform(pImpl->client); //opens and closes socket connection

        if (err == ESP_OK) {
            int statusCode = esp_http_client_get_status_code(pImpl->client);
            if (statusCode >= 200 && statusCode < 300) {
                ESP_LOGI(TAG, "Alert sent successfully (HTTP %d)", statusCode);
                return true;
            }

            ESP_LOGE(TAG, "Server responded with error status: %d", statusCode);
            return false;
        }

        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
        return false;
    }

} //namespace networking