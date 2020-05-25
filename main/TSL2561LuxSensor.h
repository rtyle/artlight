#pragma once

#include <array>
#include <cstdint>

#include "I2C.h"
#include "LuxSensor.h"

class TSL2561LuxSensor : public LuxSensor {
private:
    I2C::Master const * const i2cMaster;
    uint8_t const address;
    unsigned sensitivity;
    uint64_t startTime;

    void assertId() const;

    void setSensitivity() const;

    void start();

    void stop() const;

    uint8_t readStatus() const;

    std::array<uint16_t, 2> readChannels();
    std::array<float, 2> normalize(std::array<uint16_t, 2> channels) const;

protected:
    unsigned tillAvailable() const override;
    unsigned increaseSensitivity() override;
    unsigned decreaseSensitivity() override;
    float readLux() override;

public:
    /// i2cAddress is a function of how its address pin is wired
    enum I2cAddress: uint8_t {
        pulledLow	= 0x29,
        floating	= 0x39,
        pulledHigh	= 0x49,
    };

    TSL2561LuxSensor(
	I2C::Master const *	i2cMaster,
	uint8_t			address = floating);

    virtual ~TSL2561LuxSensor();
};
