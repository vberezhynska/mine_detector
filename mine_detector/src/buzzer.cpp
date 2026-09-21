#include "buzzer.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

static const char* TAG = "Buzzer";

namespace mine_detector {
    struct Buzzer::Impl {
        BuzzerType type;
        gpio_num_t pin;
        ledc_channel_t channel;
        ledc_timer_t timer;
        bool initialized{false};

        Impl(BuzzerType t, int p, int ch, int tm)
            : type(t),
            pin(static_cast<gpio_num_t>(p)),
            channel(static_cast<ledc_channel_t>(ch)),
            timer(static_cast<ledc_timer_t>(tm)) {}
    };

    Buzzer::Buzzer(BuzzerType type, int pin, int channel, int timer)
        : pImpl(std::make_unique<Impl>(type, pin, channel, timer)) {}

    Buzzer::Buzzer(Buzzer&&) noexcept = default;
    Buzzer& Buzzer::operator=(Buzzer&&) noexcept = default;

    Buzzer::~Buzzer() {
        turnOff();
    }

    bool Buzzer::init() {
        if (pImpl->type == BuzzerType::ACTIVE) {
            gpio_config_t io_conf = {};
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pin_bit_mask = (1ULL << pImpl->pin);
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

            esp_err_t err = gpio_config(&io_conf);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure GPIO %d for Active Buzzer", pImpl->pin);
                return false;
            }
            gpio_set_level(pImpl->pin, 0); // Ensure buzzer starts OFF
        } else {
            // Configure LEDC timer for Passive Buzzer PWM
            ledc_timer_config_t timer_conf = {};
            timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
            timer_conf.duty_resolution = LEDC_TIMER_10_BIT;
            timer_conf.timer_num = pImpl->timer;
            timer_conf.freq_hz = 2000; // Default startup frequency 2kHz
            timer_conf.clk_cfg = LEDC_AUTO_CLK;

            esp_err_t err = ledc_timer_config(&timer_conf);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure LEDC timer");
                return false;
            }

            // Configure LEDC channel
            ledc_channel_config_t channel_conf = {};
            channel_conf.gpio_num = pImpl->pin;
            channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
            channel_conf.channel = pImpl->channel;
            channel_conf.intr_type = LEDC_INTR_DISABLE;
            channel_conf.timer_sel = pImpl->timer;
            channel_conf.duty = 0; // Off by default
            channel_conf.hpoint = 0;

            err = ledc_channel_config(&channel_conf);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure LEDC channel for Passive Buzzer");
                return false;
            }
        }

        m_isSounding = false;
        ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", pImpl->pin);
        return true;
    }

    void Buzzer::turnOn() {
        if (pImpl->type == BuzzerType::ACTIVE) {
            gpio_set_level(pImpl->pin, 1);
        } else {
            playTone(1000); // Default tone
            return;
        }
        m_isSounding = true;
    }

    void Buzzer::turnOff() {
        if (pImpl->type == BuzzerType::ACTIVE) {
            gpio_set_level(pImpl->pin, 0);
        } else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, pImpl->channel, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, pImpl->channel);
        }
        m_isSounding = false;
    }

    void Buzzer::playTone(uint32_t frequencyHz) {
        if (pImpl->type == BuzzerType::ACTIVE) {
            // Active buzzers cannot produce variable frequencies; fallback to turnOn
            turnOn();
            return;
        }

        if (frequencyHz == 0) {
            turnOff();
            return;
        }

        ledc_set_freq(LEDC_LOW_SPEED_MODE, pImpl->timer, frequencyHz);
        // 50% duty cycle on 10-bit resolution (2^10 = 1024 / 2 = 512)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, pImpl->channel, 512); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, pImpl->channel);
        m_isSounding = true;
    }

    bool Buzzer::isSounding() const {
        return m_isSounding;
    }

} // namespace mine_detector