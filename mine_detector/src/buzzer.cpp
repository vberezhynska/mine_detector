#include "buzzer.hpp"
#include <iostream>

namespace mine_detector
{
    void Buzzer::init()
    {
        std::cout << "[Buzzer]  I am on!" << std::endl;
    }
} // namespace mine_detector
