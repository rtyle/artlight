#pragma once

#include <cstdint>

#include <machine/endian.h>

#include "I2C.h"
#include "MotionSensor.h"

// https://www.holtek.com/documents/10179/116711/HT7M2126_27_36_56_76v140.pdf

class HT7M2xxxMotionSensor : public MotionSensor {
private:
    I2C::Master const * const i2cMaster;
    static uint8_t constexpr address = 0b1001100;

    uint16_t getRegister(uint8_t key) const;
    void setRegister(uint8_t key, uint16_t value) const;

    uint16_t pirRawData;

public:
    HT7M2xxxMotionSensor(
	asio::io_context &	io,
	I2C::Master const *	i2cMaster);

    virtual ~HT7M2xxxMotionSensor();

    struct StandbyPeriod {
	enum : uint8_t {
	    _4ms	= 0b00,
	    _8ms	= 0b01,
	    _16ms	= 0b01,
	    _32ms	= 0b11,
	};
    };
    struct Configuration0 {
	#if BYTE_ORDER == BIG_ENDIAN
	    uint16_t	temperatureCelsius	:8;
	    uint16_t				:5;
	    uint16_t	standbyPeriod		:2;
	#else
	    uint16_t	standbyPeriod		:2;
	    uint16_t				:5;
	    uint16_t	temperatureCelsius	:8;
	#endif
	Configuration0(
	    uint8_t	standbyPeriod,
	    uint8_t	temperatureCelsius = 0);
    };
    Configuration0 getConfiguration0() const;
    void setConfiguration0(Configuration0 value) const;

    struct PirThreshold {
	enum : uint8_t {
	    _0v2	= 0b000,
	    _0v3	= 0b001,
	    _0v4	= 0b010,
	    _0v5	= 0b011,
	    _0v6	= 0b100,
	    _0v7	= 0b101,
	    _0v8	= 0b110,
	    _0v9	= 0b111,
	};
    };
    struct Lvd {
	enum : uint8_t {
	    _2v0	= 0b000,
	    _2v2	= 0b001,
	    _2v4	= 0b010,
	    _2v7	= 0b011,
	    _3v0	= 0b100,
	    _3v3	= 0b101,
	    _3v6	= 0b110,
	    _4v0	= 0b111,
	};
    };
    struct Configuration1 {
	#if BYTE_ORDER == BIG_ENDIAN
	    uint16_t	lvd		:3;
	    uint16_t	lvdEnable	:1;
	    uint16_t	pirEnable	:1;
	    uint16_t			:1;
	    uint16_t	retrigger	:1;
	    uint16_t	actEnable	:1;
	    uint16_t	pirThreshold	:3;
	    uint16_t	gain		:5;
	#else
	    uint16_t	gain		:5;
	    uint16_t	pirThreshold	:3;
	    uint16_t	actEnable	:1;
	    uint16_t	retrigger	:1;
	    uint16_t			:1;
	    uint16_t	pirEnable	:1;
	    uint16_t	lvdEnable	:1;
	    uint16_t	lvd		:3;
	#endif
	Configuration1(
	    uint8_t	gain,		//< 32 + 2 * gain
	    uint8_t	pirThreshold,
	    bool	actEnable,
	    bool	retrigger,
	    bool	pirEnable,
	    bool	lvdEnable,
	    uint8_t	lvd);
    };
    Configuration1 getConfiguration1() const;
    void setConfiguration1(Configuration1 value) const;

    struct Configuration2 {
	#if BYTE_ORDER == BIG_ENDIAN
	    uint8_t	luminanceThreshold;
	    uint8_t	address;
	#else
	    uint8_t	address;
	    uint8_t	luminanceThreshold;
	#endif
	    Configuration2(
	    uint8_t	address,
	    uint8_t	luminanceThreshold);
    };
    Configuration2 getConfiguration2() const;
    void setConfiguration2(Configuration2 value) const;

    /// 100ms unit
    uint16_t getTriggerTimeInterval() const;
    void setTriggerTimeInterval(uint16_t value) const;

    uint16_t getPirRawData() const;
    uint16_t getOpticalSensorRawData() const;
    uint16_t getTemperatureSensorRawData() const;

    struct Status {
	#if BYTE_ORDER == BIG_ENDIAN
	    uint16_t	initializing		:1;
	    uint16_t				:6;
	    uint16_t	lvd			:1;
	    uint16_t	dark			:1;
	    uint16_t				:4;
	    uint16_t	pirNoiseDetected	:1;
	    uint16_t	pirTriggeredAgain	:1;
	    uint16_t	pirTriggered		:1;
	#else
	    uint16_t	pirTriggered		:1;
	    uint16_t	pirTriggeredAgain	:1;
	    uint16_t	pirNoiseDetected	:1;
	    uint16_t				:4;
	    uint16_t	dark			:1;
	    uint16_t	lvd			:1;
	    uint16_t				:6;
	    uint16_t	initializing		:1;
	#endif
    };
    Status getStatus() const;

    uint16_t getManufacturerId() const;
    uint16_t getFirmwareVersion() const;

    void assertId() const;

    unsigned tillAvailable() const override;
    unsigned increaseSensitivity() override;
    unsigned decreaseSensitivity() override;
    /// implementations may throw
    /// std::underflow_error (increaseSensitivity) or
    /// std::overflow_error (decreaseSensitivity)
    float readMotion() override;
};
