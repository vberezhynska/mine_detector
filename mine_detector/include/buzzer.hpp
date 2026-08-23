#pragma once

#include "IDevice.hpp"

//Buzzer (GPIO 5): Uses ESP-IDF's driver/gpio.h (active buzzer)
namespace mine_detector {
    class Buzzer : IDevice {
        public:
        void init();
    };
}; //namespace Controller