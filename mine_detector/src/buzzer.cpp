#include "buzzer.hpp"
#include "esp_log.h"

static const char* TAG = "Buzzer";

namespace mine_detector {
    Buzzer::Buzzer(gpio_num_t pin, BuzzerType type, ledc_channel_t channel, ledc_timer_t timer)
        : m_pin(pin), m_type(type), m_channel(channel), m_timer(timer) {}

    Buzzer::~Buzzer() {
        turnOff();
    }

    bool Buzzer::init() {
        if (m_type == BuzzerType::ACTIVE) {
            gpio_config_t io_conf = {};
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pin_bit_mask = (1ULL << m_pin);
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

            esp_err_t err = gpio_config(&io_conf);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure GPIO %d for Active Buzzer", m_pin);
                return false;
            }
            gpio_set_level(m_pin, 0); // Ensure buzzer starts OFF
        } else {
            // Configure LEDC timer for Passive Buzzer PWM
            ledc_timer_config_t timer_conf = {};
            timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
            timer_conf.duty_resolution = LEDC_TIMER_10_BIT;
            timer_conf.timer_num = m_timer;
            timer_conf.freq_hz = 2000; // Default startup frequency 2kHz
            timer_conf.clk_cfg = LEDC_AUTO_CLK;

            esp_err_t err = ledc_timer_config(&timer_conf);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure LEDC timer");
                return false;
            }

            // Configure LEDC channel
            ledc_channel_config_t channel_conf = {};
            channel_conf.gpio_num = m_pin;
            channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
            channel_conf.channel = m_channel;
            channel_conf.intr_type = LEDC_INTR_DISABLE;
            channel_conf.timer_sel = m_timer;
            channel_conf.duty = 0; // Off by default
            channel_conf.hpoint = 0;

            err = ledc_channel_config(&channel_conf);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure LEDC channel for Passive Buzzer");
                return false;
            }
        }

        m_isSounding = false;
        ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", m_pin);
        return true;
    }

    void Buzzer::turnOn() {
        if (m_type == BuzzerType::ACTIVE) {
            gpio_set_level(m_pin, 1);
        } else {
            playTone(2000); // Default tone
            return;
        }
        m_isSounding = true;
    }

    void Buzzer::turnOff() {
        if (m_type == BuzzerType::ACTIVE) {
            gpio_set_level(m_pin, 0);
        } else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, m_channel, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, m_channel);
        }
        m_isSounding = false;
    }

    void Buzzer::playTone(uint32_t frequencyHz) {
        if (m_type == BuzzerType::ACTIVE) {
            // Active buzzers cannot produce variable frequencies; fallback to turnOn
            turnOn();
            return;
        }

        if (frequencyHz == 0) {
            turnOff();
            return;
        }

        ledc_set_freq(LEDC_LOW_SPEED_MODE, m_timer, frequencyHz);
        // 50% duty cycle on 10-bit resolution (2^10 = 1024 / 2 = 512)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m_channel, 512); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, m_channel);
        m_isSounding = true;
    }

    bool Buzzer::isSounding() const {
        return m_isSounding;
    }

} // namespace mine_detector