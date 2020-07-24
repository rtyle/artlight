#pragma once

#include <atomic>

#include <asio.hpp>

#include "Timer.h"

class MotionSensor {
protected:
    char const * const	name;
private:
    asio::io_context &	io;
    Timer		timer;
    std::atomic<float>	motion;

    void update();

protected:
    MotionSensor(char const * name, asio::io_context & io);
    virtual ~MotionSensor();
    /// implementations must return milliseconds till available
    virtual unsigned tillAvailable() const = 0;
    virtual unsigned increaseSensitivity() = 0;
    virtual unsigned decreaseSensitivity() = 0;
    /// implementations may throw
    /// std::underflow_error (increaseSensitivity) or
    /// std::overflow_error (decreaseSensitivity)
    virtual float readMotion() = 0;

public:
    float getMotion();
};