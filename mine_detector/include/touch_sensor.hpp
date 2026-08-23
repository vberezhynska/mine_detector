#pragma once

#include "driver/gpio.h"
#include "IDevice.hpp"

//should be on GPIO 4, reads digital pin state HIGH/LOW
namespace mine_detector {
    class TouchSensor : IDevice {
        public:
            explicit TouchSensor(gpio_num_t pin = GPIO_NUM_4);
            ~TouchSensor() override = default;
            bool init() override;
            bool isTouched() const; // Returns true if touched (GPIO HIGH/active)

        private:
            gpio_num_t m_pin;
            bool m_initialized{false};
    };
}; //namespace Controller