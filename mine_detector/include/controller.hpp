#pragma once

#include <memory>

#include "IController.hpp"
#include "buzzer.hpp"
#include "touch_sensor.hpp"
#include "gps_neo.hpp"

namespace networking { class UdpSocket; };

namespace mine_detector {
    class Controller : IController {
        public:
            explicit Controller(TouchSensor& sensor, 
                Buzzer& buzzer, 
                GpsNeo& gps, 
                const std::unique_ptr<networking::UdpSocket>& udp_socket,
                uint32_t pollIntervalMs = 500);
            ~Controller();

            Controller(const Controller&) = delete;
            Controller& operator=(const Controller&) = delete;
            Controller(Buzzer&&) noexcept;
            Controller& operator=(Controller&&) noexcept;

            bool start() override;
            void stop() override;
            bool isRunning();

        private:
            struct Impl;
            std::unique_ptr<Impl> pImpl;

            static void task_wrapper(void* arg);
            void runLoop();
    };
}; //namespace Controller