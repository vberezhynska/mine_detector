#pragma once

namespace Location { 
    struct Point;
    // Abstract Interface
    class IGeoLogic {
    public:
        // Virtual destructor is crucial for proper cleanup of derived objects
        virtual ~IGeoLogic() = default;

        // Pure virtual functions (must be overridden by concrete classes)
        virtual void isInRange(Location::Point point) = 0;
        virtual Location::Point getMiddlePoint(Location::Point point) = 0;
    };
} //namespace Location

