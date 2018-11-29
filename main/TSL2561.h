#ifndef TSL2561_h__
#define TSL2561_h__

#include <cstdint>

#include "I2C.h"

class TSL2561 {
public:
    /// i2cAddress is a function of how its address pin is wired
    enum I2cAddress: uint8_t {
        pulledLow	= 0x29,
        floating	= 0x39,
        pulledHigh	= 0x49,
    };

    enum IntegrationTime {
	fastest	= 0,	///< 402ms * 11/322
	faster	= 1,	///< 402ms * 81/322
	normal	= 2,	///< 402ms
	manual	= 3,	///< set/delay/unset manualStart
    };

private:
    I2C::Master const * const i2cMaster;
    uint8_t const	address;
    IntegrationTime	integrationTime;
    bool		gainTimes16;
    bool		manualStart;

public:
    TSL2561(
	I2C::Master const *	i2cMaster,
	uint8_t			address,
	IntegrationTime		integrationTime	= normal,
	bool			gainTimes16	= false,
	bool			manualStart	= false);

    void start() const;

    void stop() const;

    uint8_t getId() const;

    void setTiming(
	IntegrationTime		integrationTime	= normal,
	bool			gainTimes16	= false,
	bool			manualStart	= false);

    /// return the milliseconds integrationTime according to the current timing.
    unsigned getIntegrationTime();

    template<typename T = uint16_t>
    struct Channels {
	T _[2];
    };

    /// return channel measurements from the device
    Channels<> getChannels() const;

    /// return normalized channel measurements
    /// (comparable to that of a normal IntegrationTime with a gainTime16)
    /// according to the current timing.
    /// a non-zero manualTime must be provided if the
    /// current integrationTime is manual.
    /// this should be the number of milliseconds between the set/unset
    /// of a manualStart.
    Channels<float> getNormalizedChannels(unsigned manualTime = 1) const;

    /// convert the normalized channels to a lux measurement of ambient light
    /// according to the TSL2561 product datasheet
    /// https://cdn-shop.adafruit.com/datasheets/TSL2561.pdf
    float getLux(unsigned manualTime = 1) const;

    virtual ~TSL2561();
};

#endif
