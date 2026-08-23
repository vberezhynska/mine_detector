# pragma once

// Abstract Interface

namespace mine_detector
{
    class IController {
    public:
        // Virtual destructor is crucial for proper cleanup of derived objects
        virtual ~IController() = default;

        // Pure virtual functions (must be overridden by concrete classes)
        virtual void start() = 0;
    };
} // namespace mine_detector

