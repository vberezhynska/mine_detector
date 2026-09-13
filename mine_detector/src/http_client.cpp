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

    bool HttpClient::parseMineAlertResponse() {
        char response_buffer[256] = {0};
        int read_len = esp_http_client_read_response(pImpl->client, response_buffer, sizeof(response_buffer) - 1);
        
        if (read_len <= 0) {
            ESP_LOGE(TAG, "No response body received (read_len=%d)", read_len);
            return false;
        }

        response_buffer[read_len] = '\0';
        auto parsed = json::parse(response_buffer, nullptr, false);

        if (parsed.is_discarded() || !parsed.contains("status") || !parsed["status"].is_string()) {
            ESP_LOGE(TAG, "Failed to parse JSON or 'status' field missing: %s", response_buffer);
            return false;
        }

        if (parsed["status"] == "success") {
            ESP_LOGI(TAG, "Alert validated by server: success");
            return true;
        }

        ESP_LOGE(TAG, "Server responded with status: %s", parsed["status"].get<std::string>().c_str());
        if (parsed.contains("error") && parsed["error"].is_string()) {
            ESP_LOGE(TAG, "Error detail: %s", parsed["error"].get<std::string>().c_str());
        }
        return false;
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
        //TODO: add port to base url
        std::string fullUrl = pImpl->baseUrl + ":" + std::to_string(pImpl->port) + pImpl->alert_url;

        ESP_LOGI(TAG, "HTTP sendMineAlert will send to fullUrl: %s", 
             fullUrl.c_str());

        esp_http_client_set_url(pImpl->client, fullUrl.c_str());
        esp_http_client_set_method(pImpl->client, HTTP_METHOD_POST);
        esp_http_client_set_header(pImpl->client, "Content-Type", "application/json");
        esp_http_client_set_header(pImpl->client, "Connection", "close"); //close socket connection
        esp_http_client_set_post_field(pImpl->client, payload.c_str(), static_cast<int>(payload.length()));

        esp_err_t err = esp_http_client_perform(pImpl->client); //opens and closes socket connection

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
            esp_http_client_close(pImpl->client);
            return false;
        }

        int statusCode = esp_http_client_get_status_code(pImpl->client);
        if (statusCode >= 200 && statusCode < 300 && parseMineAlertResponse()) { 
            ESP_LOGI(TAG, "Alert sent successfully (HTTP %d)", statusCode);
            esp_http_client_close(pImpl->client);
            return true;
        }
        
        ESP_LOGE(TAG, "Server responded with error status: %d", statusCode);
        esp_http_client_close(pImpl->client);
        return false;
    }

} //namespace networking