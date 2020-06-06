#pragma once

#include <array>
#include <cstdint>

#include "I2C.h"
#include "LuxSensor.h"

class TSL2591LuxSensor : public LuxSensor {
private:
    I2C::Master const * const i2cMaster;
    static uint8_t constexpr address = 0x29;
    unsigned sensitivity;

    void assertId() const;

    void setSensitivity() const;

    void start();

    void stop() const;

    uint8_t readStatus() const;

    std::array<uint16_t, 2> readChannels();

protected:
    unsigned tillAvailable() const override;
    unsigned increaseSensitivity() override;
    unsigned decreaseSensitivity() override;
    float readLux() override;

public:
    TSL2591LuxSensor(
	asio::io_context &	io,
	I2C::Master const *	i2cMaster);

    virtual ~TSL2591LuxSensor();
};
