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

public:
    HT7M2xxxMotionSensor(
	asio::io_context &	io,
	I2C::Master const *	i2cMaster);

    virtual ~HT7M2xxxMotionSensor();

    struct PirPeriod {
	enum : uint8_t {
	    _4ms	= 0b00,
	    _8ms	= 0b01,
	    _16ms	= 0b10,
	    _32ms	= 0b11,
	};
    };
    static unsigned constexpr pirPeriodDecode(unsigned encoding) {
	return 1 << (2 + encoding);
    }
    struct Configuration0 {
	union {
	    uint16_t	encoding;
	    struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		signed		temperatureCelsius	:8;
		unsigned				:6;
		unsigned	pirPeriod		:2;
	    #else
		signed		temperatureCelsius	:8;
		unsigned	pirPeriod		:2;
		unsigned				:6;
	    #endif
	    };
	};
	Configuration0(uint16_t encoding);
	Configuration0(
	    signed	temperatureCelsius,
	    unsigned	pirPeriod);
	#define setter(name) Configuration0 & name##_(decltype(name) s);
	setter(temperatureCelsius)
	setter(pirPeriod)
	#undef setter
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
    static float constexpr pirThresholdDecode(unsigned encoding) {
	return (2.0f + encoding) / 10.0f;
    }
    struct LowVoltageDetectThreshold {
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
	union {
	    uint16_t	encoding;
	    struct {
		#if BYTE_ORDER == BIG_ENDIAN
		    unsigned	lowVoltageDetectThreshold	:3;
		    bool	lowVoltageDetectEnable		:1;
		    bool	pirDetectEnable			:1;
		    bool					:1;
		    bool	pirRetrigger			:1;
		    bool	pirTriggeredPinEnable		:1;
		    unsigned	pirThreshold			:3;
		    unsigned	pirGain				:5;
		#else
		    bool	pirTriggeredPinEnable		:1;
		    bool	pirRetrigger			:1;
		    bool					:1;
		    bool	pirDetectEnable			:1;
		    bool	lowVoltageDetectEnable		:1;
		    unsigned	lowVoltageDetectThreshold	:3;
		    unsigned	pirGain				:5;
		    unsigned	pirThreshold			:3;
		#endif
	    };
	};
	Configuration1(uint16_t encoding);
	Configuration1(
	    unsigned	lowVoltageDetectThreshold,
	    bool	lowVoltageDetectEnable,
	    bool	pirDetectEnable,
	    bool	pirRetrigger,
	    bool	pirTriggeredPinEnable,
	    unsigned	pirThreshold,
	    unsigned	pirGain);
	#define setter(name) Configuration1 & name##_(decltype(name) s);
	setter(lowVoltageDetectThreshold)
	setter(lowVoltageDetectEnable)
	setter(pirDetectEnable)
	setter(pirRetrigger)
	setter(pirTriggeredPinEnable)
	setter(pirThreshold)
	setter(pirGain)
	#undef setter
    };
    static unsigned constexpr pirGainDecode(unsigned encoding) {
	return 32 + 2 * encoding;
    }
    Configuration1 getConfiguration1() const;
    void setConfiguration1(Configuration1 value) const;

    struct Configuration2 {
	union {
	    uint16_t	encoding;
	    struct {
		#if BYTE_ORDER == BIG_ENDIAN
		    unsigned	darkThreshold		:7;
		    bool	pirDetectIfDarkEnable	:1;
		    unsigned	address			:7;
		    bool				:1;
		#else
		    bool	pirDetectIfDarkEnable	:1;
		    unsigned	darkThreshold		:7;
		    bool				:1;
		    unsigned	address			:7;
		#endif
	    };
	};
	Configuration2(uint16_t encoding);
	Configuration2(
	    unsigned	darkThreshold,	///< vs. optical sensor a/d
	    bool	pirDetectIfDarkEnable,
	    unsigned	address);
	#define setter(name) Configuration2 & name##_(decltype(name) s);
	setter(darkThreshold)
	setter(pirDetectIfDarkEnable)
	setter(address)
	#undef setter
    };
    Configuration2 getConfiguration2() const;
    void setConfiguration2(Configuration2 value) const;

    /// 100ms unit
    unsigned getDuration() const;
    void setDuration(unsigned value) override;

    unsigned getPirRawData() const;
    unsigned getOpticalSensorRawData() const;
    unsigned getTemperatureSensorRawData() const;

    struct Status {
	union {
	    uint16_t	encoding;
	    struct {
		#if BYTE_ORDER == BIG_ENDIAN
		    bool	notReady		:1;
		    unsigned				:6;
		    bool	lowVoltageDetected	:1;
		    bool	dark			:1;
		    unsigned				:4;
		    bool	pirNoiseDetected	:1;
		    bool	pirRetriggered		:1;
		    bool	pirTriggered		:1;
		#else
		    bool	lowVoltageDetected	:1;
		    unsigned				:6;
		    bool	notReady		:1;
		    bool	pirTriggered		:1;
		    bool	pirRetriggered		:1;
		    bool	pirNoiseDetected	:1;
		    unsigned				:4;
		    bool	dark			:1;
		#endif
	    };
	};
	Status(uint16_t encoding);
    };
    Status getStatus() const;

    unsigned getManufacturerId() const;
    unsigned getFirmwareVersion() const;

    void assertId() const;

    unsigned getPeriod() const override;
    bool readMotion() const override;
};
