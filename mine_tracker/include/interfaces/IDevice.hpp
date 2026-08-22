#pragma once

// Abstract Interface
class IDevice {
public:
    // Virtual destructor is crucial for proper cleanup of derived objects
    virtual ~IDevice() = default;

    // Pure virtual functions (must be overridden by concrete classes)
    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
    virtual bool isRunning() const = 0;
};