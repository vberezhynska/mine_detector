#pragma once

#include "IController.hpp"

namespace mine_detector {
    class ESP32 : IController {
        public:
            void start();
    };
}; //namespace Controller