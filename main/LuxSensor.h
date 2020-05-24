#pragma once

class LuxSensor {
public:
    LuxSensor();
    virtual ~LuxSensor();
    /// implementations must return milliseconds till available
    virtual unsigned tillAvailable() const = 0;
    virtual unsigned increaseSensitivity() = 0;
    virtual unsigned decreaseSensitivity() = 0;
    /// implementations may throw
    /// std::underflow_error (increaseSensitivity) or
    /// std::overflow_error (decreaseSensitivity)
    virtual float readLux() const = 0;
};
