#pragma once

#include "IDevice.hpp"

//should be on GPIO 4, reads digital pin state HIGH/LOW
namespace mine_detector {
    class TouchSensor : IDevice {
        public:
            void init();
    };
}; //namespace Controller