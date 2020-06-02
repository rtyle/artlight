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
    {0b00,	normalIntegrationTime * 11.0f / 322.0f	,  4900}, // 5047},
    {0b01,	normalIntegrationTime * 81.0f / 322.0f	, 37000}, // 37177},
    {0b10,	normalIntegrationTime			, 65000}, // 65535},
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
		uint8_t				:2;
		uint8_t	gain			:1;
		uint8_t	manualStart		:1;
		uint8_t				:1;
		uint8_t	integrationTime	:2;
	    #else
		uint8_t	integrationTime	:2;
		uint8_t				:1;
		uint8_t	manualStart		:1;
		uint8_t	gain			:1;
		uint8_t				:2;
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
	    manualStart	(false),
	    integrationTime	(integrationTime_)
	#else
	    integrationTime	(integrationTime_),
	    manualStart	(false),
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
		uint8_t	partNumber		:4;
		uint8_t	revisionNumber	:4;
	    #else
		uint8_t	revisionNumber	:4;
		uint8_t	partNumber		:4;
	    #endif
	};
    };
    Id(uint8_t encoding_) : encoding(encoding_) {}
    operator uint8_t() const {return encoding;}
};

TSL2561LuxSensor::TSL2561LuxSensor(
    asio::io_context &	io_,
    I2C::Master const *	i2cMaster_,
    uint8_t		address_)
:
    LuxSensor	{"TSL2561", io_},
    i2cMaster	{i2cMaster_},
    address	{address_},
    sensitivity	{0},
    startTime	{0}
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

void TSL2561LuxSensor::start() {
    i2cMaster->commands(address, wait)
	.writeByte(ControlCommand())
	.writeByte(Control(true));
    startTime = esp_timer_get_time();
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

std::array<uint16_t, 2> TSL2561LuxSensor::readChannels() {
    auto pair = sensitivities.pairs[sensitivity];
    if (1000 * pair.first.value > esp_timer_get_time() - startTime) {
	throw std::underflow_error("TSL2561 not available");
    }
    std::array<uint16_t, 2> raw;
    auto i = 0;
    for (auto & e: raw) {
	i2cMaster->commands(address, wait)
	    .writeByte(ChannelCommand(i++))
	    .startRead()
	    .readBytes(&e, sizeof e);
	#if BYTE_ORDER == BIG_ENDIAN
	    e = __builtin_bswap16(e);
	#endif
	if (0 < sensitivity && e >= pair.first.overflow) {
	    throw std::overflow_error("TSL2561 too sensitive");
	}
	if (sensitivities.pairs.size() > sensitivity + 1 && 0 == e) {
	    throw std::underflow_error("TSL2561 too insensitive");
	}
    }
    return raw;
}

std::array<float, 2> TSL2561LuxSensor::normalize(
    std::array<uint16_t, 2> raw) const
{
    std::array<float, 2> normalized;
    auto pair = sensitivities.pairs[sensitivity];
    float normalize = normalIntegrationTime * normalGain
	/ pair.first.value / pair.second.value;
    for (auto i = 0; i < normalized.size(); ++i) {
	normalized[i] = normalize * raw[i];
    }
    return normalized;
}

float TSL2561LuxSensor::readLux() {
    // "Calculating Lux" for the TSL2561 T package from ...
    // https://ams.com/documents/20143/36005/TSL2561_DS000110_3-00.pdf
    // ... seems to have an error in its first "segment"
    // as that is the only one that does not factor in CH1.
    // this error is propagated in the SparkFun implementation
    // https://github.com/sparkfun/SparkFun_TSL2561_Arduino_Library/blob/master/src/SparkFunTSL2561.cpp
    // Contrarily, "Calculating Lux for the TSL2561" from
    // https://ams.com/documents/20143/36005/AmbientLightSensors_AN000170_2-00.pdf
    // expands this first segment into four -
    // all of which do factor in CH1.
    // a fixed point variation of this is what is used
    // in the Adafruit implementation
    // https://github.com/adafruit/Adafruit_TSL2561/blob/master/Adafruit_TSL2561_U.cpp
    // we use the original floating point variant here.
    std::array<uint16_t, 2> raw {readChannels()};
    std::array<float, 2> ch {normalize(raw)};
    float ratio {std::numeric_limits<float>::infinity()};
    float lux {0.0f};
    if (ch[0]) {
	ratio = ch[1] / ch[0];
	lux =
		0.125f >= ratio ? 0.03040f * ch[0] - 0.02720f * ch[1]
	:	0.250f >= ratio ? 0.03250f * ch[0] - 0.04400f * ch[1]
	:	0.375f >= ratio ? 0.03510f * ch[0] - 0.05440f * ch[1]
	:	0.500f >= ratio ? 0.03750f * ch[0] - 0.06240f * ch[1]
	:	0.610f >= ratio ? 0.02240f * ch[0] - 0.03100f * ch[1]
	:	0.800f >= ratio ? 0.01280f * ch[0] - 0.01530f * ch[1]
	:	1.300f >= ratio ? 0.00146f * ch[0] - 0.00112f * ch[1]
	:	0.0;
    }
#if 0
    auto pair = sensitivities.pairs[sensitivity];
    ESP_LOGI("TSL2561", "lux %f\traw %d\t%d\tsensitivity %d\ttime %f\tgain %f\tch %f\t%f\tratio %f",
	lux,
	raw[0], raw[1],
	sensitivity, pair.first.value, pair.second.value,
	ch[0], ch[1],
	ratio);
#endif
    return lux;
}

TSL2561LuxSensor::~TSL2561LuxSensor() {
    stop();
}
