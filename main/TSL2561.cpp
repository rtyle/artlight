#include <cmath>

#include <machine/endian.h>

#include "TSL2561.h"

#include "dump.h"

enum Register: uint8_t {
    control		= 0x0,
    timing		= 0x1,
    lowThresholdLow	= 0x2,
    lowThresholdHigh	= 0x3,
    highThresholdLow	= 0x4,
    highThresholdHigh	= 0x5,
    interrupt		= 0x6,
    crc			= 0x8,
    id			= 0xa,
    channel0Low		= 0xc,
    channel0High	= 0xd,
    channel1Low		= 0xe,
    channel1High	= 0xf,
};

struct Command {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		bool		command	:1;
		bool		clear	:1;
		bool		word	:1;
		bool		block	:1;
		unsigned	address	:4;
	    #else
		unsigned	address	:4;
		bool		block	:1;
		bool		word	:1;
		bool		clear	:1;
		bool		command	:1;
	    #endif
	};
    };
protected:
    Command(
	uint8_t	address_,
	bool	block_	= false,
	bool	word_	= false,
	bool	clear_	= false)
    :
	#if BYTE_ORDER == BIG_ENDIAN
	    command	(true),
	    clear	(clear_),
	    word	(word_),
	    block	(block_),
	    address	(address_)
	#else
	    address	(address_),
	    block	(block_),
	    word	(word_),
	    clear	(clear_),
	    command	(true)
	#endif
    {}
public:
    operator uint8_t() const {return encoding;}
};

struct ControlCommand : public Command {
public:
    ControlCommand() : Command(Register::control) {}
};

struct TimingCommand : public Command {
public:
    TimingCommand() : Command(Register::timing) {}
};

struct IdCommand : public Command {
public:
    IdCommand() : Command(Register::id) {}
};

struct ChannelCommand : public Command {
public:
    ChannelCommand(bool one) :
	Command(one
	    ? Register::channel1Low
	    : Register::channel0Low, false, true) {}
};

struct ChannelsCommand : public Command {
public:
    ChannelsCommand() : Command(0xb, true) {}
};

struct Control {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		unsigned		:6;
		unsigned	power	:2;
	    #else
		unsigned	power	:2;
		unsigned		:6;
	    #endif
	};
    };
public:
    Control(bool power_)
    :
	power(power_ ? ~0 : 0)
    {}
    operator uint8_t() const {return encoding;}
};

struct Timing {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		unsigned			:2;
		unsigned	gainTimes16	:1;
		unsigned	manualStart	:1;
		unsigned			:1;
		unsigned	integrationTime	:2;
	    #else
		unsigned	integrationTime	:2;
		unsigned			:1;
		unsigned	manualStart	:1;
		unsigned	gainTimes16	:1;
		unsigned			:2;
	    #endif
	};
    };
public:
    Timing(
	TSL2561::IntegrationTime	integrationTime_,
	bool				gainTimes16_,
	bool				manualStart_)
    :
	#if BYTE_ORDER == BIG_ENDIAN
	    gainTimes16		(gainTimes16_),
	    manualStart		(manualStart_),
	    integrationTime	(integrationTime_)
	#else
	    integrationTime	(integrationTime_),
	    manualStart		(manualStart_),
	    gainTimes16		(gainTimes16_)
	#endif
    {}
    operator uint8_t() const {return encoding;}
};

struct Id {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		unsigned	partNumber	:4;
		unsigned	revisionNumber	:4;
	    #else
		unsigned	revisionNumber	:4;
		unsigned	partNumber	:4;
	    #endif
	};
    };
public:
    Id(uint8_t encoding_) : encoding(encoding_) {}
    operator uint8_t() const {return encoding;}
};

TSL2561::TSL2561(
    I2C::Master const *	i2cMaster_,
    uint8_t		address_,
    IntegrationTime	integrationTime_,
    bool		gainTimes16_,
    bool		manualStart_)
:
    i2cMaster		(i2cMaster_),
    address		(address_),
    integrationTime	(integrationTime_),
    gainTimes16		(gainTimes16_),
    manualStart		(manualStart_)
{
    setTiming(integrationTime, gainTimes16, manualStart);
    start();
}

// how long we should wait for commands to complete
static TickType_t constexpr wait = 1;

void TSL2561::start() const {
    i2cMaster->commands(address, wait)
	.writeByte(ControlCommand())
	.writeByte(Control(true));
}

void TSL2561::stop() const {
    i2cMaster->commands(address, wait)
	.writeByte(ControlCommand())
	.writeByte(Control(false));
}

uint8_t TSL2561::getId() const {
    uint8_t id;
    i2cMaster->commands(address, wait)
	.writeByte(IdCommand())
	.startRead()
	.readByte(&id)
    ;
    return id;
}

void TSL2561::setTiming(
    IntegrationTime	integrationTime_,
    bool		gainTimes16_,
    bool		manualStart_)
{
    integrationTime	= integrationTime_;
    gainTimes16		= gainTimes16_;
    manualStart		= manualStart_;
    i2cMaster->commands(address, wait)
	.writeByte(TimingCommand())
	.writeByte(Timing(integrationTime, gainTimes16, manualStart));
}

static float normalIntegrationTime = 402.0;

// scales with respect to normalIntegrationTime
static float constexpr normalizeFrom[] = {
    322.0 / 11.0,	// 0, fastest
    322.0 / 81.0,	// 1, faster
    1.0			// 2, normal
};

unsigned TSL2561::getIntegrationTime() {
    return manual == integrationTime
	? 0
	: normalIntegrationTime / normalizeFrom[integrationTime];
}

TSL2561::Channels<> TSL2561::getChannels() const {
    struct {
	uint8_t		align;
	uint8_t		byteCount;
	Channels<>	channels;
    } result {};
    #if 0
	// read channels at once.
	// although the ChannelsCommand used for doing this is documented,
	//	* it is not documented well (channel order is reversed!?)
	//	* it is not used in the documented example
	//	* it is not used in Adafruit library code
	//	* it does not work as well with noisy i2c lines
	i2cMaster->commands(address, wait)
	    .writeByte(ChannelsCommand())
	    .startRead()
	    .readBytes(&result.byteCount,
		sizeof result.byteCount + sizeof result.channels)
	;
	uint16_t   channel1  = result.channels._[0];
	result.channels._[0] = result.channels._[1];
	result.channels._[1] = channel1;
    #else
	// read channels separately
	auto i = 0;
	for (auto & e: result.channels._) {
	    i2cMaster->commands(address, wait)
		.writeByte(ChannelCommand(i))
		.startRead()
		.readBytes(&e, sizeof e);
	    i ^= 1;
	}
    #endif
    #if BYTE_ORDER == BIG_ENDIAN
	for (auto & e: result.getChannels._) e = __builtin_bswap16(e);
    #endif
    return result.channels;
}

TSL2561::Channels<float> TSL2561::getNormalizedChannels(
    unsigned manualTime) const
{
    Channels<> unnormalized = getChannels();
    Channels<float> normalized;
    float normalize = manual == integrationTime
	? normalIntegrationTime / manualTime
	: normalizeFrom[integrationTime];
    if (!gainTimes16) normalize *= 16;
    for (auto i = 0; i < sizeof normalized._ / sizeof *normalized._; i++) {
	normalized._[i] = normalize * unnormalized._[i];
    }
    return normalized;
}

float TSL2561::getLux(unsigned manualTime) const {
    Channels<float> ch = getNormalizedChannels(manualTime);
    if (!ch._[0]) return 0;
    float ratio = ch._[1] / ch._[0];
    return
    #if 0
    // TSL2561 CS package
      0.52 >= ratio ? 0.03150 * ch._[0] - 0.05930 * ch._[0] * std::pow(ratio, 1.4)
    : 0.65 >= ratio ? 0.02290 * ch._[0] - 0.02910 * ch._[1]
    : 0.80 >= ratio ? 0.01570 * ch._[0] - 0.01800 * ch._[1]
    : 1.30 >= ratio ? 0.00338 * ch._[0] - 0.00260 * ch._[1]
    :                 0.0
    #else
    // TSL2561 T, FN or CL package
      0.50 >= ratio ? 0.03040 * ch._[0] - 0.06200 * ch._[0] * std::pow(ratio, 1.4)
    : 0.61 >= ratio ? 0.02240 * ch._[0] - 0.03100 * ch._[1]
    : 0.80 >= ratio ? 0.01280 * ch._[0] - 0.01530 * ch._[1]
    : 1.30 >= ratio ? 0.00146 * ch._[0] - 0.00112 * ch._[1]
    :                 0.0
    #endif
    ;
}

TSL2561::~TSL2561() {
    stop();
}
