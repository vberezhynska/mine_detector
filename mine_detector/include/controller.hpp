#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "IController.hpp"
#include "buzzer.hpp"
#include "touch_sensor.hpp"
#include "gps_neo.hpp"

namespace mine_detector {
    class Controller : IController {
        public:
            Controller(TouchSensor& sensor, Buzzer& buzzer, GpsNeo& gps, uint32_t pollIntervalMs = 500);
            ~Controller();
            bool start();
            void stop() override;
            bool isRunning() const { return m_isRunning; }

        private:
            TouchSensor& m_sensor;
            Buzzer& m_buzzer;
            GpsNeo& m_gps;
            uint32_t m_pollIntervalMs;
            TaskHandle_t m_taskHandle{nullptr};
            bool m_isRunning{false};

            void runInSimpleLoop();
    };
}; //namespace Controller