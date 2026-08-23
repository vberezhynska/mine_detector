#include "touch_sensor.hpp"
#include "esp_log.h"

static const char* TAG = "TouchSensor";

namespace mine_detector {

TouchSensor::TouchSensor(gpio_num_t pin) : m_pin(pin) {}

bool TouchSensor::init() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << m_pin);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d", m_pin);
        return false;
    }
    
    return true;
}

bool TouchSensor::isTouched() const {
    int level = gpio_get_level(m_pin);
    return (level == 1);
}

} // namespace mine_detector