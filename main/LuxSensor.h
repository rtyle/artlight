#pragma once

#include <atomic>

#include <asio.hpp>

#include "Timer.h"

class LuxSensor {
protected:
    char const * const	name;
private:
    asio::io_context &	io;
    Timer		timer;
    std::atomic<float>	lux;

    void update();

protected:
    LuxSensor(char const * name, asio::io_context & io);
    virtual ~LuxSensor();
    /// implementations must return milliseconds till available
    virtual unsigned tillAvailable() const = 0;
    virtual unsigned increaseSensitivity() = 0;
    virtual unsigned decreaseSensitivity() = 0;
    /// implementations may throw
    /// std::underflow_error (increaseSensitivity) or
    /// std::overflow_error (decreaseSensitivity)
    virtual float readLux() = 0;

public:
    float getLux();
};
