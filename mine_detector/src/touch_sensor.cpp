#include "touch_sensor.hpp"
#include "esp_log.h"
#include "driver/gpio.h"

static const char* TAG = "TouchSensor";

namespace mine_detector {

struct TouchSensor::Impl {
    gpio_num_t pin;
    bool initialized{false};

    explicit Impl(gpio_num_t p) : pin(p) {}
};

TouchSensor::TouchSensor(int pin) : pImpl(std::make_unique<Impl>(static_cast<gpio_num_t>(pin))) {}

TouchSensor::~TouchSensor() = default;

TouchSensor::TouchSensor(TouchSensor&&) noexcept = default;
TouchSensor& TouchSensor::operator=(TouchSensor&&) noexcept = default;

bool TouchSensor::init() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << pImpl->pin);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d", pImpl->pin);
        return false;
    }
    
    pImpl->initialized = true;
    return true;
}

bool TouchSensor::isTouched() const {
    int level = gpio_get_level(pImpl->pin);
    return (level == 1);
}

} // namespace mine_detector