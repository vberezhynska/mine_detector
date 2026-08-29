#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "IDevice.hpp"

//Buzzer (GPIO 5): Uses ESP-IDF's driver/gpio.h (active buzzer)
namespace mine_detector {

enum class BuzzerType {
    ACTIVE,  // Simple ON/OFF GPIO high/low control
    PASSIVE  // PWM tone generation via LEDC peripheral
};

class Buzzer {
public:
    /**
     * @param pin GPIO pin connected to the buzzer
     * @param type ACTIVE or PASSIVE buzzer mode
     * @param channel LEDC channel used for passive mode PWM (default: LEDC_CHANNEL_0)
     * @param timer LEDC timer used for passive mode PWM (default: LEDC_TIMER_0)
     */
    explicit Buzzer(gpio_num_t pin, 
                    BuzzerType type = BuzzerType::ACTIVE,
                    ledc_channel_t channel = LEDC_CHANNEL_0,
                    ledc_timer_t timer = LEDC_TIMER_0);

    ~Buzzer();

    bool init();
    void turnOn();
    void turnOff();
    void playTone(uint32_t frequencyHz);
    bool isSounding() const;

    private:
        gpio_num_t m_pin;
        BuzzerType m_type;
        ledc_channel_t m_channel;
        ledc_timer_t m_timer;
        bool m_isSounding{false};
    };

} // namespace mine_detector