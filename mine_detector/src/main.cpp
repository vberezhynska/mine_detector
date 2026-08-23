#include <iostream>
#include "buzzer.hpp"
#include "touch_sensor.hpp"
#include "controller.hpp"

int main(int argc, char* argv[]) {
    std::cout << "[Mine detector] I am working!" << std::endl;
    mine_detector::ESP32 controller {};
    controller.start();
    
    mine_detector::TouchSensor sensor {};
    sensor.init();

    mine_detector::Buzzer buzzer {};
    buzzer.init();

    return 0;
}