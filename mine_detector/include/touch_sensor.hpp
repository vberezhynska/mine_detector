#pragma once

#include "IDevice.hpp"
#include <memory>

namespace mine_detector {

class TouchSensor : public IDevice {
public:
    explicit TouchSensor(int pin = 4);
    ~TouchSensor() override;

    TouchSensor(const TouchSensor&) = delete;
    TouchSensor& operator=(const TouchSensor&) = delete;
    TouchSensor(TouchSensor&&) noexcept;
    TouchSensor& operator=(TouchSensor&&) noexcept;

    bool init() override;
    bool isTouched() const; // Returns true if touched (GPIO HIGH/active)

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace mine_detector