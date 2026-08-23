#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "IController.hpp"
#include "buzzer.hpp"
#include "touch_sensor.hpp"

namespace mine_detector {
    class Controller : IController {
        public:
            Controller(TouchSensor& sensor, Buzzer& buzzer, uint32_t pollIntervalMs = 500);
            ~Controller();
            bool start() override;
            void stop() override;
            bool isRunning() const { return m_isRunning; }

        private:
            static void taskWrapper(void* arg);
            void runLoop();

            TouchSensor& m_sensor;
            Buzzer& m_buzzer;
            uint32_t m_pollIntervalMs;
            TaskHandle_t m_taskHandle{nullptr};
            bool m_isRunning{false};
    };
}; //namespace Controller