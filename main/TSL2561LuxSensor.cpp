#include "TSL2561LuxSensor.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include <machine/endian.h>

#include <esp_log.h>

// https://ams.com/tsl2561

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
    ControlCommand() : Command{Register::control} {}
};

struct TimingCommand : public Command {
public:
    TimingCommand() : Command{Register::timing} {}
};

struct IdCommand : public Command {
public:
    IdCommand() : Command{Register::id} {}
};

struct ChannelCommand : public Command {
public:
    ChannelCommand(bool one) :
	Command{one
	    ? Register::channel1Low
	    : Register::channel0Low} {}
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

struct IntegrationTime {
    uint8_t	encoding;
    float	value;
    unsigned	overflow;
};

static float constexpr normalIntegrationTime {402.0f};

static std::array<IntegrationTime, 3> constexpr integrationTimes {{
    {0b00,	normalIntegrationTime * 11.0f / 322.0f	,  5047},
    {0b01,	normalIntegrationTime * 81.0f / 322.0f	, 37177},
    {0b10,	normalIntegrationTime			, 65535},
}};

struct Gain {
    uint8_t	encoding;
    float	value;
};

static float constexpr normalGain {16.0f};

static std::array<Gain, 2> constexpr gains {{
    {0b0,	 1.0f},
    {0b1,	16.0f},
}};

// the cartesian product of two sets is the set of all pairs
template<typename I, typename J, typename O>
void cartesianProduct(I const & is, J const & js, O o) {
    for (auto i = std::begin(is); i != std::end(is); ++i) {
	for (auto j = std::begin(js); j != std::end(js); ++j) {
	    *o++ = std::make_pair(*i, *j);
	}
    }
}

// our sensitivities instance contains sorted pairs of integrationTime and gain
// encodings in increasing value product order.
static struct Sensitivities {
    using Pair = std::pair<IntegrationTime, Gain>;
    std::vector<Pair> pairs;
    Sensitivities() {
	cartesianProduct(integrationTimes, gains, std::back_inserter(pairs));
	std::sort(pairs.begin(), pairs.end(),
	    [](Pair const & a, Pair const & b) {
		return a.first.value * a.second.value
		     < b.first.value * b.second.value;
	    }
	);
    }
} const sensitivities;

struct Timing {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		uint8_t			:2;
		uint8_t	gain		:1;
		uint8_t	manualStart	:1;
		uint8_t			:1;
		uint8_t	integrationTime	:2;
	    #else
		uint8_t	integrationTime	:2;
		uint8_t			:1;
		uint8_t	manualStart	:1;
		uint8_t	gain		:1;
		uint8_t			:2;
	    #endif
	};
    };
public:
    Timing(
	uint8_t	integrationTime_,
	uint8_t	gain_)
    :
	#if BYTE_ORDER == BIG_ENDIAN
	    gain		(gain_),
	    manualStart		(false),
	    integrationTime	(integrationTime_)
	#else
	    integrationTime	(integrationTime_),
	    manualStart		(false),
	    gain		(gain_)
	#endif
    {}
    operator uint8_t() const {return encoding;}
};

struct Id {
public:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		uint8_t	partNumber	:4;
		uint8_t	revisionNumber	:4;
	    #else
		uint8_t	revisionNumber	:4;
		uint8_t	partNumber	:4;
	    #endif
	};
    };
    Id(uint8_t encoding_) : encoding(encoding_) {}
    operator uint8_t() const {return encoding;}
};

TSL2561LuxSensor::TSL2561LuxSensor(
    I2C::Master const *	i2cMaster_,
    uint8_t		address_)
:
    LuxSensor		{},
    i2cMaster		{i2cMaster_},
    address		{address_},
    sensitivity		{0}
{
    assertId();
    setSensitivity();
    start();
}

// how long we should wait for commands to complete
static TickType_t constexpr wait = 1;

void TSL2561LuxSensor::assertId() const {
    uint8_t id;
    i2cMaster->commands(address, wait)
	.writeByte(IdCommand())
	.startRead()
	.readByte(&id)
    ;
    assert(id == 0x50);
}

void TSL2561LuxSensor::setSensitivity() const {
    auto pair = sensitivities.pairs[sensitivity];
    i2cMaster->commands(address, wait)
	.writeByte(TimingCommand())
	.writeByte(Timing(pair.first.encoding, pair.second.encoding));
}

void TSL2561LuxSensor::start() const {
    i2cMaster->commands(address, wait)
	.writeByte(ControlCommand())
	.writeByte(Control(true));
}

void TSL2561LuxSensor::stop() const {
    i2cMaster->commands(address, wait)
	.writeByte(ControlCommand())
	.writeByte(Control(false));
}

unsigned TSL2561LuxSensor::tillAvailable() const {
    return sensitivities.pairs[sensitivity].first.value;
}

unsigned TSL2561LuxSensor::increaseSensitivity() {
    if (sensitivities.pairs.size() > sensitivity + 1) {
	++sensitivity;
	setSensitivity();
	stop();
	start();
    }
    return tillAvailable();
}

unsigned TSL2561LuxSensor::decreaseSensitivity() {
    if (0 < sensitivity) {
	--sensitivity;
	setSensitivity();
	stop();
	start();
    }
    return tillAvailable();
}

std::array<uint16_t, 2> TSL2561LuxSensor::readChannels() const {
    std::array<uint16_t, 2> channels;
    auto i = 0;
    for (auto & e: channels) {
	i2cMaster->commands(address, wait)
	    .writeByte(ChannelCommand(i++))
	    .startRead()
	    .readBytes(&e, sizeof e);
    }
    #if BYTE_ORDER == BIG_ENDIAN
	for (auto & e: channels._) e = __builtin_bswap16(e);
    #endif
    return channels;
}

std::array<float, 2> TSL2561LuxSensor::normalize(
    std::array<uint16_t, 2> unnormalized) const
{
    std::array<float, 2> normalized;
    auto pair = sensitivities.pairs[sensitivity];
    float normalize =
	normalIntegrationTime * normalGain / pair.first.value / pair.second.value;
    for (auto i = 0; i < normalized.size(); ++i) {
	if (unnormalized[i] >= pair.first.overflow) {
	    throw std::overflow_error("TSL2561");
	}
	normalized[i] = normalize * unnormalized[i];
    }
#if 0
    ESP_LOGI("TSL2561", "%d\t%d\t%d\t%d\t%d\t%f\t%f",
	sensitivity,
	pair.first.encoding, pair.second.encoding,
	unnormalized[0], unnormalized[1],
	normalized[0], normalized[1]);
#endif
    return normalized;
}

float TSL2561LuxSensor::readLux() const {
    std::array<float, 2> ch = normalize(readChannels());
    if (!ch[0]) return 0.0f;
    float ratio = ch[1] / ch[0];
    return
    #if 0
    // TSL2561 CS package
      0.52f >= ratio ? 0.03150f * ch[0] - 0.05930f * ch[0] * std::pow(ratio, 1.4f)
    : 0.65f >= ratio ? 0.02290f * ch[0] - 0.02910f * ch[1]
    : 0.80f >= ratio ? 0.01570f * ch[0] - 0.01800f * ch[1]
    : 1.30f >= ratio ? 0.00338f * ch[0] - 0.00260f * ch[1]
    :                 0.0
    #else
    // TSL2561 T, FN or CL package
      0.50f >= ratio ? 0.03040f * ch[0] - 0.06200f * ch[0] * std::pow(ratio, 1.4f)
    : 0.61f >= ratio ? 0.02240f * ch[0] - 0.03100f * ch[1]
    : 0.80f >= ratio ? 0.01280f * ch[0] - 0.01530f * ch[1]
    : 1.30f >= ratio ? 0.00146f * ch[0] - 0.00112f * ch[1]
    :                 0.0
    #endif
    ;
}

TSL2561LuxSensor::~TSL2561LuxSensor() {
    stop();
}
