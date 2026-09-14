#pragma once
#include <cstdint>

// Abstract Interface
class IGeoLogic {
public:
    // Virtual destructor is crucial for proper cleanup of derived objects
    virtual ~IGeoLogic() = default;

    // Pure virtual functions (must be overridden by concrete classes)
    virtual uint8_t confirmationsNumber() = 0;
    virtual bool isConfirirmed() = 0;
    virtual bool isInConfirmedList() = 0;
};