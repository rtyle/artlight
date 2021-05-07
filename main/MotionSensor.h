#pragma once

#include <atomic>

#include "asio.hpp"

#include "Timer.h"

class MotionSensor {
protected:
    char const * const	name;
private:
    asio::io_context &	io;
    std::atomic<bool>	motion;

    void update();

protected:
    Timer		timer;

    MotionSensor(char const * name, asio::io_context & io);
    /// implementations must return polling period in ticks
    virtual unsigned getPeriod() const = 0;
    virtual bool readMotion() const = 0;

public:
    /// overrides must call MotionSensor::setDuration
    /// and hold motion state for duration, in deciseconds
    virtual void setDuration(unsigned value);

    bool getMotion();
    virtual ~MotionSensor();
};
