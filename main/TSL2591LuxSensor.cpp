#include "TSL2591LuxSensor.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

#include <machine/endian.h>

#include <esp_log.h>

// https://ams.com/tsl25911

// the data sheet uses the term "persistence" to give significance to
// an intensity change that been sustained for a configurable amount of time.
// unfortunately, they often use it in the negative sense as in "no persist"
// (a change has not persisted) or not at all (otherwise).
// this is confusing.
// instead, we use the term "stable" instead of "persistent"
// and "transient" instead of "non-persistent" or "immediate".
//
// the data sheet also uses the acronym ALS (Ambient Light Sensor?)
// without defining it.
// we say, simply, "sensor".

// scope of everything in this anonymous namespace
// is limited to this translation unit and has internal linkage
namespace {

enum Register: uint8_t {
    enable			= 0x00,
    control			= 0x01,
    stableLowThresholdLow	= 0x04,
    stableLowThresholdHigh	= 0x05,
    stableHighThresholdLow	= 0x06,
    stableHighThresholdHigh	= 0x07,
    transientLowThresholdLow	= 0x08,
    transientlowThresholdHigh	= 0x09,
    transientHighThresholdLow	= 0x0a,
    transientHighThresholdHigh	= 0x0b,
    stability			= 0x0c,
    packageId			= 0x11,
    id				= 0x12,
    status			= 0x13,
    channel0Low			= 0x14,
    channel0High		= 0x15,
    channel1Low			= 0x16,
    channel1High		= 0x17,
};

enum Function: uint8_t {
    interruptSet		= 0b00100,
    interruptClearStable	= 0b00110,
    intertuptClearBoth		= 0b00111,
    interruptClearTransient	= 0b01010,
};

enum Transaction: uint8_t {
    register_	= 0b01,
    function	= 0b11,
};

struct Command {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		uint8_t	command		:1;
		uint8_t	transaction	:2
		uint8_t	address		:5;
	    #else
		uint8_t	address		:5;
		uint8_t	transaction	:2;
		uint8_t	command		:1;
	    #endif
	};
    };
protected:
    Command(
	uint8_t	address_,
	uint8_t transaction_ = register_)
    :
	#if BYTE_ORDER == BIG_ENDIAN
	    command	(1),
	    transaction	(transaction_),
	    address	(address_)
	#else
	    address	{address_},
	    transaction	{transaction_},
	    command	{1}
	#endif
    {}
public:
    operator uint8_t() const {return encoding;}
};

struct EnableCommand : public Command {
public:
    EnableCommand() : Command{Register::enable} {}
};

struct ControlCommand : public Command {
public:
    ControlCommand() : Command{Register::control} {}
};

struct IdCommand : public Command {
public:
    IdCommand() : Command{Register::id} {}
};

struct StatusCommand : public Command {
public:
    StatusCommand() : Command{Register::status} {}
};

struct ChannelCommand : public Command {
public:
    ChannelCommand(bool one) :
	Command{one
	    ? Register::channel1Low
	    : Register::channel0Low} {}
};

struct Enable {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		uint8_t		transientInterrupt	:1;
		uint8_t		sleepAfterInterrupt	:1;
		uint8_t					:1;
		uint8_t		stableInterrupt		:1;
		uint8_t					:1;
		uint8_t		sensor			:1;
		uint8_t		oscillator		:1;
	    #else
		uint8_t		oscillator		:1;
		uint8_t		sensor			:1;
		uint8_t					:1;
		uint8_t		stableInterrupt		:1;
		uint8_t					:1;
		uint8_t		sleepAfterInterrupt	:1;
		uint8_t		transientInterrupt	:1;
	    #endif
	};
    };
public:
    Enable(
	bool oscillator_		= true,
	bool sensor_			= true,
	bool stableInterrupt_		= false,
	bool sleepAfterInterrupt_	= false,
	bool transientInterrupt_	= false)
    :
	oscillator		{oscillator_},
	sensor			{sensor_},
	stableInterrupt		{stableInterrupt_},
	sleepAfterInterrupt	{sleepAfterInterrupt_},
	transientInterrupt	{transientInterrupt_}
    {}
    operator uint8_t() const {return encoding;}
};

struct IntegrationTime {
    uint8_t	encoding;
    float	value;
    unsigned	overflow;
};

static float constexpr normalIntegrationTime = 400.0f;

static std::array<IntegrationTime, 6> constexpr integrationTimes {{
    {0b000,	100.0f,	36863},
    {0b001,	200.0f, 65535},
    {0b010,	300.0f, 65535},
    {0b011,	400.0f, 65535},
    {0b100,	500.0f, 65535},
    {0b101,	600.0f, 65535},
}};

struct Gain {
    uint8_t	encoding;
    float	value;
};

static float constexpr normalGain {16.0f};

static std::array<Gain, 4> constexpr gains {{
    {0b00,	   1.0f},
    {0b01,	  24.5f},
    {0b10,	 400.0f},
    {0b11,	9200.0f},
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

struct Control {
private:
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		uint8_t	reset		:1;
		uint8_t			:1;
		uint8_t	gain		:2;
		uint8_t			:1;
		uint8_t	integrationTime	:3;
	    #else
		uint8_t	integrationTime	:3;
		uint8_t			:1;
		uint8_t	gain		:2;
		uint8_t			:1;
		uint8_t	reset		:1;
	    #endif
	};
    };
public:
    Control(
	uint8_t	integrationTime_,
	uint8_t	gain_,
	bool	reset_ = false)
    :
	#if BYTE_ORDER == BIG_ENDIAN
	    reset		{reset_},
	    gain		{gain_},
	    integrationTime	{integrationTime_}
	#else
	    integrationTime	{integrationTime_},
	    gain		{gain_},
	    reset		{reset_}
	#endif
    {}
    operator uint8_t() const {return encoding;}
};

struct Status {
    union {
	uint8_t	encoding;
	struct {
	    #if BYTE_ORDER == BIG_ENDIAN
		uint8_t				:2;
		uint8_t transientInterrupt	:1;
		uint8_t stableInterrupt		:1;
		uint8_t				:3;
		uint8_t available		:1;

	    #else
		uint8_t available		:1;
		uint8_t				:3;
		uint8_t stableInterrupt		:1;
		uint8_t transientInterrupt	:1;
		uint8_t				:2;
	    #endif
	};
    };
    Status(uint8_t encoding_) : encoding {encoding_} {}
};

}

TSL2591LuxSensor::TSL2591LuxSensor(
    asio::io_context &	io_,
    I2C::Master const *	i2cMaster_)
:
    LuxSensor		{"TSL2591", io_},
    i2cMaster		{i2cMaster_},
    sensitivity		{0}
{
    assertId();
    setSensitivity();
    start();
    timer.start();
}

// how long we should wait for commands to complete
static TickType_t constexpr wait = 1;

void TSL2591LuxSensor::assertId() const {
    uint8_t id;
    i2cMaster->commands(address, wait)
	.writeByte(IdCommand())
	.startRead()
	.readByte(&id)
    ;
    assert(id == 0x50);
}

void TSL2591LuxSensor::setSensitivity() const {
    auto pair = sensitivities.pairs[sensitivity];
    i2cMaster->commands(address, wait)
	.writeByte(ControlCommand())
	.writeByte(Control(pair.first.encoding, pair.second.encoding));
}

void TSL2591LuxSensor::start() {
    i2cMaster->commands(address, wait)
	.writeByte(EnableCommand())
	.writeByte(Enable(true, true));
}

void TSL2591LuxSensor::stop() const {
    i2cMaster->commands(address, wait)
	.writeByte(EnableCommand())
	.writeByte(Enable(false, false));
}

uint8_t TSL2591LuxSensor::readStatus() const {
    uint8_t status;
    i2cMaster->commands(address, wait)
	.writeByte(StatusCommand())
	.startRead()
	.readByte(&status)
    ;
    return status;
}

unsigned TSL2591LuxSensor::tillAvailable() const {
    return sensitivities.pairs[sensitivity].first.value;
}

unsigned TSL2591LuxSensor::increaseSensitivity() {
    if (sensitivities.pairs.size() > sensitivity + 1) {
	++sensitivity;
	setSensitivity();
	stop();
	start();
    }
    return tillAvailable();
}

unsigned TSL2591LuxSensor::decreaseSensitivity() {
    if (0 < sensitivity) {
	--sensitivity;
	setSensitivity();
	stop();
	start();
    }
    return tillAvailable();
}

std::array<uint16_t, 2> TSL2591LuxSensor::readChannels() {
    Status status {readStatus()};
    if (!status.available) {
	throw std::underflow_error("TSL2591 not available");
    }
    auto pair = sensitivities.pairs[sensitivity];
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
	    throw std::overflow_error("TSL2591 too sensitive");
	}
	if (sensitivities.pairs.size() > sensitivity + 1 && 0 == e) {
	    throw std::underflow_error("TSL2591 too insensitive");
	}
    }
    return raw;
}

// per Norwood.Brown@ams.com
//
//	If there is no glass above the sensor like in our EVM kit
//	or if you use clear glass which does not attenuate the light
//	and does not change the spectrum,
//	then you can use this "open air" lux equation from our lab:
//
//	Lux1 = DGF * (Ch0 - (coefB * Ch1)) / (ATime * AGain)
//	Lux2 = DGF * ((coefC * Ch0) - (coefD * Ch1)) / (ATime * AGain)
//	Lux = MAX(Lux1, Lux2, 0)
//
//	DGF = 923
//	CoefB = 3.15
//	CoefC = 0.46
//	CoefD = 0.74
//
// see also,
//	https://www.desmos.com/calculator/zzdqmobcb3

static float factor(unsigned sensitivity) {
    static float constexpr dgf {923.0f};
    auto const pair = sensitivities.pairs[sensitivity];
    return dgf						// DGF
	/ (pair.first.value * pair.second.value);	// / (ATime * AGain)
}

static float lux1(float factor, std::array<uint16_t, 2> ch) {
    static float constexpr coefB {3.15f};
    return factor * ((		ch[0]) - (coefB * ch[1]));
}

static float lux2(float factor, std::array<uint16_t, 2> ch) {
    static float constexpr coefC {0.46f};
    static float constexpr coefD {0.74f};
    return factor * ((coefC *	ch[0]) - (coefD * ch[1]));
}

float TSL2591LuxSensor::readLux() {
    std::array<uint16_t, 2> ch = readChannels();
    float const factor	= ::factor(sensitivity);
    float const lux1	= ::lux1(factor, ch);
    float const lux2	= ::lux2(factor, ch);
    float lux		= std::max(lux1, lux2);
    if (0 > lux) {
	// less than 0 lux, in the real world, is impossible.
	// there was no exceptional condition (e.g. underflow or overflow)
	// that we can blame this on.
	// instead, we will return 0 in call cases except
	// where the device is configured to be the least sensitive it can be
	// (presumbably, because its latest experience was very bright light).
	// in this case, we return the brightest value that we could possibly
	// measure in that least-sensitive state
	static std::array<uint16_t, 2> constexpr chBrightest {
	    integrationTimes[0].overflow, 0};
	static float const luxBrightest {::lux1(::factor(0), chBrightest)};
	lux = 0 == sensitivity ? luxBrightest : 0.0f;
    }
#if 0
    auto const pair = sensitivities.pairs[sensitivity];
    ESP_LOGI(name,
	"lux %f\traw %d\t%d\tsensitivity %d\ttime %f\tgain %f\tlux1 %f\tlux2 %f",
	lux,
	ch[0], ch[1],
	sensitivity, pair.first.value, pair.second.value,
	lux1, lux2);
#endif
    return lux;
}

TSL2591LuxSensor::~TSL2591LuxSensor() {
    stop();
}
