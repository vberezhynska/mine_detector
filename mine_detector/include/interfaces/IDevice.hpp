#pragma once

namespace mine_detector
{
    class IDevice {
    public:
        // Virtual destructor is crucial for proper cleanup of derived objects
        virtual ~IDevice() = default;

        // Pure virtual functions (must be overridden by concrete classes)
        virtual void init() = 0;
    };
} // namespace mine_detector


