#pragma once

#include <memory>

#include "IDevice.hpp"


//Buzzer (GPIO 5): Uses ESP-IDF's driver/gpio.h (active buzzer)
namespace mine_detector {

enum class BuzzerType {
    ACTIVE,  // Simple ON/OFF GPIO high/low control
    PASSIVE  // PWM tone generation via LEDC peripheral
};

class Buzzer : IDevice {
public:
    /**
     * @param pin GPIO pin connected to the buzzer
     * @param type ACTIVE or PASSIVE buzzer mode
     * @param channel LEDC channel used for passive mode PWM (default: LEDC_CHANNEL_0)
     * @param timer LEDC timer used for passive mode PWM (default: LEDC_TIMER_0)
     */
    explicit Buzzer(BuzzerType type = BuzzerType::ACTIVE,
                    int pin = 5,
                    int channel = 0,
                    int timer = 0);
    ~Buzzer() override;

    Buzzer(const Buzzer&) = delete;
    Buzzer& operator=(const Buzzer&) = delete;
    Buzzer(Buzzer&&) noexcept;
    Buzzer& operator=(Buzzer&&) noexcept;

    bool init() override;
    void turnOn();
    void turnOff();
    void playTone(uint32_t frequencyHz);
    bool isSounding() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
        bool m_isSounding{false};
    };

} // namespace mine_detector