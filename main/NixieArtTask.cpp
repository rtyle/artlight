#include <cmath>
#include <limits>
#include <numeric>

#include <esp_log.h>

#define APA102_RGB
#include "APA102.h"
#undef APA102_RGB

#include "Curve.h"
#include "NixieArtTask.h"
#include "TSL2591LuxSensor.h"
#include "Timer.h"
#include "fromString.h"

extern "C" uint64_t get_time_since_boot();

using APA102::LED;
using LEDI = APA102::LED<int>;

static unsigned constexpr millisecondsPerSecond	{1000u};
static unsigned constexpr microsecondsPerSecond	{1000000u};

static SawtoothCurve inNearSecondOf	{0.0f,  1.0f * 60.0f / 61.0f};
static SawtoothCurve inSecondOf		{0.0f,  1.0f};
static SawtoothCurve inNearBiSecondOf	{0.0f,  2.0f * 30.0f / 31.0f};
static SawtoothCurve inBiSecondOf	{0.0f,  2.0f};
static SawtoothCurve inMinuteOf		{0.0f, 60.0f};
static SawtoothCurve inHourOf		{0.0f, 60.0f * 60.0f};
static SawtoothCurve inDayOf		{0.0f, 60.0f * 60.0f * 12.0f};	/// 12 hour clock
static SawtoothCurve inDigitsOf		{0.0f, 10.0f};

char const * const NixieArtTask::Mode::string[]
    {"clock", "count", "roll", "clean"};
NixieArtTask::Mode::Mode(Value value_) : value(value_) {}
NixieArtTask::Mode::Mode(char const * value) : value(
    [value](){
	size_t i {0};
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return clock;
    }()
) {}
char const * NixieArtTask::Mode::toString() const {
    return string[value];
}

/// A HasDigit instance is used to iterate over a range of numbers
/// where each has a digit at a place in the base (default 10) presentation.
/// For example
/// for (auto i: HasDigit(60, 5, 0)) std::cout << i << std::endl;
/// will print 5, 15, 25, 35, 45 and 55 whereas
/// for (auto i: HasDigit(60, 5, 1)) std::cout << i << std::endl;
/// will print 50, 51, 52, 53, 54, 55, 56, 57, 58, 59.
class HasDigit {
private:
    unsigned const range;
    unsigned const digit;
    unsigned const place;
    unsigned const radix;
    unsigned value(unsigned digit, unsigned place, unsigned index) const {
	if (digit || index) {
	    if (place--) {
		return index % radix + radix * value(digit, place, index / radix);
	    } else {
		return digit + radix * value(0, place, index);
	    }
	} else {
	    return 0;
	}
    }
    unsigned value(unsigned index) const {
	return value(digit, place, index);
    }
public:
    class Iterator: public std::iterator<std::input_iterator_tag, unsigned,
	unsigned, unsigned const *, unsigned> {
    private:
	HasDigit const & hasDigit;
	unsigned value;
	unsigned index;
    public:
	explicit Iterator(HasDigit const & hasDigit_, unsigned index_)
	:
	    hasDigit {hasDigit_},
	    value {index_ == std::numeric_limits<unsigned>::max()
		? 0
		: hasDigit.value(index_)},
	    index {value < hasDigit.range
		? index_
		: std::numeric_limits<unsigned>::max()}
	{};
	Iterator & operator++() {
	    value = hasDigit.value(++index);
	    if (!(value < hasDigit.range))
		index = std::numeric_limits<unsigned>::max();
	    return *this;
	}
	bool operator==(Iterator that) const {
	    return index == that.index;
	}
	bool operator!=(Iterator that) const {
	    return !(*this == that);
	}
	reference operator*() const {return value;}
    };
    HasDigit(unsigned range_, unsigned digit_, unsigned place_, unsigned radix_ = 10)
    :
	range	{range_},
	digit	{digit_ % radix_},
	place	{place_},
	radix	{radix_ }
    {}
    Iterator begin() {return Iterator(*this, 0);}
    Iterator end() {return Iterator(*this, std::numeric_limits<unsigned>::max());}
};

static float digitize0(
    unsigned			range,
    unsigned			digit,
    unsigned			place,
    std::function<float(float)> f,
    unsigned			radix = 10)
{
    float result {0.0f};
    for (auto e: HasDigit(range, digit, place, radix)) {
	result += f(e / static_cast<float>(range));
    }
    return std::min(1.0f, result);
}

static float digitize1(
    unsigned			range,
    unsigned			digit,
    unsigned			place,
    std::function<float(float)> f,
    unsigned			radix = 10)
{
    float result {0.0f};
    for (auto e: HasDigit(range + 1, digit, place, radix)) {
	if (0 < e) result += f(e / static_cast<float>(range));
    }
    return std::min(1.0f, result);
}

static unsigned snapOffNixie(unsigned cathode, unsigned value) {
    // the default PCA9685 prescale value (30)
    // combined with its internal oscillator (25 MHz)
    // results in a 200 Hz update rate for its 4096 PWM values.
    //	30 = std::round(25000000 / (4096 * 200)) - 1
    // This results in a minimum PWM on time of 1.22 microseconds.
    // https://www.youtube.com/watch?v=TK3E55fytC0
    // suggests that a "warm" nixie tube
    // *in their circuit* can operate at 100000 Hz
    // (on time of 10 microseconds with a turn on time of less than 5).
    // it would take an update rate of 48.83 Hz to to guarantee
    // that the minimal PWM value (1) would turn on a nixie tube.
    // such a low update rate, however, would likely cause flickering.
    //
    // it is better to keep the default 200 Hz update rate and
    // snap off sub-minimal values that have been
    // seen to not properly light the cathodes of the tubes in our circuit.
    static unsigned constexpr min[] {
	30,	// 0
	16,	// 1
	38,	// 2
	42,	// 3
	28,	// 4
	50,	// 5
	26,	// 6
	28,	// 7
	36,	// 8
	26,	// 9
	26,	// dot
    };
    return min[cathode] > value ? 0 : value;
}

static float bias(unsigned cathode) {
    // 8 is the dimmest cathode.
    // bias others relative to it based on evaluation of our circuit/tubes.
    static float constexpr min[] {
	0.8750f,	// 0
	0.5000f,	// 1
	0.7500f,	// 2
	0.8125f,	// 3
	0.6875f,	// 4
	0.8125f,	// 5
	0.7500f,	// 6
	0.6250f,	// 7
	1.0000f,	// 8
	0.7500f,	// 9
	0.8125f,	// dot
    };
    return min[cathode];
}

static float fade(float high, float dim, float min, float from) {
    if (1.0f <= from || 0.0f == dim || min > high) {
	return high;		// snap high
    } else {
	if (0.0f == from && 1.0 == dim) {
	    return 0.0f;	// snap off
	} else {
	    auto const low {std::max(min, high * (1.0f - dim))};
	    return low + (high - low) * from;
	}
    }
}

void NixieArtTask::update_() {
    // construct the default (off) LED and nixie image
    APA102::Message<ledCount> ledMessage;
    PCA9685::Pwm pca9685Pwms[pca9685s.size()][PCA9685::pwmCount];

    if (!motionSensor || motionSensor->getMotion()) {
	// clip lux blacks and whites for value to fade from
	float lux {luxSensor ? luxSensor->getLux() : white};
	if (black > lux) lux = 0.0f;
	float const fadeFrom {lux < white ? lux / white : 1.0f};

	float constexpr minLed {4.0f /  256.0f}; // for close color
	float const fadeLeds[sideCount] {
	    fade(levels[0], dims[0], minLed, fadeFrom),
	    fade(levels[1], dims[1], minLed, fadeFrom),
	};

	float constexpr minNixie {64.0 / 4096.0f}; // will not snap off even after digit bias
	float const fadeNixie {fade(levels[2], dims[2], minNixie, fadeFrom)};

	for (auto & e : ledMessage.encodings) {
	    auto side {1 & (&e - ledMessage.encodings)};
	    e = colors[side] * fadeLeds[side];
	}

	uint64_t const microsecondsSinceBoot {get_time_since_boot()};

	std::function<float(unsigned)> placeValues[pca9685s.size()] {
	    [](unsigned) {return 0.0f;},
	    [](unsigned) {return 0.0f;},
	    [](unsigned) {return 0.0f;},
	    [](unsigned) {return 0.0f;},
	};
	std::function<float(unsigned)> dots {
	    [](unsigned) {return 0.0f;}
	};

	// Cathode Poisoning Prevention Routine
	// http://docs.daliborfarny.com/doc/r-z568m-nixie-tube/#1550
	// recommends any-on:minimum-on ratio of no more than 300:1
	// Normal clock displays digits in places
	//	0. 10:1	{0-9}	1 minute  in 10
	//	1. 60:0	{6-9}	0 minutes in 60
	//	2. 12:1	{0,3-9}	1 hour  in 12
	//	3.  3:0	{0,2-9}	0 hours in 3 (9 hours in 12 is off)
	// Unused digits in {1,3} places must be cleaned.

	// sputter from used cathodes may "poison"/dirty others.
	// this sputter will be burned off when those others are used.
	// these are the potential unused/dirtyPlaces when left in clock mode.
	static std::array<std::vector<unsigned>, placeCount> const dirtyPlaces {{
	    {1, 0, 2, 3, 9, 4, 8, 5, 7, 6},	// all used equally
	    {            9,    8,    7, 6},	// only 0-5 used
	    {   0,    3, 9, 4, 8, 5, 7, 6},	// 1 and 2 used twice as much as others
	    {   0, 2, 3, 9, 4, 8, 5, 7, 6},	// only 1 used
	}};

	switch (mode.value) {
	case Mode::count : {
		auto placeValue {placeValues};
		float /* deciseconds */ sinceModeChange {
		    (microsecondsSinceBoot - microsecondsSinceBootOfModeChange)
		    / 100000.0f
		};
		unsigned order {4};
		for (unsigned placeIndex = pca9685s.size(); placeIndex--;) {
		    MesaDial dial {inDigitsOf(sinceModeChange), 1.0f / 10.0f, order};
		    *placeValue++ = [dial](unsigned digit) {
			return digitize0(10, digit, 0, dial);
		    };
		    sinceModeChange /= 10.0f;
		    order *= 10;
		}
	    } break;
	case Mode::roll : {
		auto placeValue {placeValues};
		float const /* seconds */ sinceModeChange {
		    (microsecondsSinceBoot - microsecondsSinceBootOfModeChange)
		    / 1000000.0f
		};
		unsigned order {4};
		for (unsigned placeIndex = pca9685s.size(); placeIndex--;) {
		    MesaDial dial {inDigitsOf(sinceModeChange), 1.0f / 10.0f, order};
		    *placeValue++ = [dial](unsigned digit) {
			return digitize0(10, digit, 0, dial);
		    };
		}
	    } break;
	case Mode::clean : {
		auto placeValue {placeValues};
		unsigned const /* decisecond */ counter
		    {static_cast<unsigned>(10.0f * smoothTime.millisecondsSinceTwelveLocaltime(
			    microsecondsSinceBoot)
			/ static_cast<float>(millisecondsPerSecond))};
		for (auto & dirtyPlace: dirtyPlaces) {
		    unsigned const value {dirtyPlace[counter % dirtyPlace.size()]};
		    *placeValue++ = [value](unsigned digit) {
			return value == digit ? 1.0f : 0.0f;
		    };
		}
	    } break;
	case Mode::clock :
	default : {
		auto placeValue {placeValues};
		float const secondsSinceTwelveLocaltime
		    {smoothTime.millisecondsSinceTwelveLocaltime(
			    microsecondsSinceBoot)
			/ static_cast<float>(millisecondsPerSecond)};
		float const inDay {inDayOf(secondsSinceTwelveLocaltime)};
		MesaDial hourDial {inDay, 1.0f / 12.0f, 4 * 60 * 60};
		float const inHour {inHourOf(secondsSinceTwelveLocaltime)};
		MesaDial minuteDial {inHour, 1.0f / 60.0f, 4 * 60};

		// clean dirtyPlaces time is the first minute of each hour
		unsigned const /* decisecond inHour */ counter
		    {static_cast<unsigned>(inHour * 60.0f * 60.0f * 10.0f)};
		bool const clean {600 > counter};

		unsigned placeIndex {0};
		for (auto & dirtyPlace: dirtyPlaces) {
#if 0
		    // any-on:minimum-on ratios (compare with above)
		    //	1. 480  :1 {6-9}	 1/ 8 minute in  60
		    //	3. 283.5:1 {0,2-9}	12/18 minute in 189

		    // split the cleaning time in halves
		    bool const thatHalf {300 > counter};
		    // clean half the places at a time
		    bool const thisHalf {placeIndex >> 1};
		    if (clean && thisHalf == thatHalf) {
			unsigned const value {dirtyPlace[counter % dirtyPlace.size()]};
			*placeValue++ = [value](unsigned digit) {
			    return value == digit ? 1.0f : 0.0f;
			};
		    } else {
			if (thisHalf == thatHalf) {
			    // display minutes in this half
			    switch (1 & placeIndex) {
			    case 1:
				*placeValue++ = [minuteDial](unsigned digit) {
				    return digitize0(60, digit, 1, minuteDial);
				};
				break;
			    case 0:
				*placeValue++ = [minuteDial](unsigned digit) {
				    return digitize0(60, digit, 0, minuteDial);
				};
				break;
			    }
			} else {
			    // display hours in this half
			    switch (1 & placeIndex) {
			    case 1:
				*placeValue++ = [hourDial](unsigned digit) {
				    auto const value {digitize1(12, digit, 1, hourDial)};
				    return 0 == digit && 0.0f < value ? 0.0f : value;
				};
				break;
			    case 0:
				*placeValue++ = [hourDial](unsigned digit) {
				    return digitize1(12, digit, 0, hourDial);
				};
				break;
			    }
			}
		    }
#else
		    // any-on:minimum-on ratios (compare with above)
		    //	1. 240  :1	{6-9}	1/ 4 minute in  60
		    //	3. 189  :1	{0,2-9}	1    minute in 189
		    if (clean && (2 > placeIndex || (2 < placeIndex &&
			    1.0f / 12.0f <= inDay && 10.0f / 12.0f > inDay))) {
			unsigned const value {dirtyPlace[counter % dirtyPlace.size()]};
			*placeValue++ = [value](unsigned digit) {
			    return value == digit ? 1.0f : 0.0f;
			};
		    } else {
			switch (placeIndex) {
			case 3:
			    *placeValue++ = [hourDial](unsigned digit) {
				auto const value {digitize1(12, digit, 1, hourDial)};
				return 0 == digit && 0.0f < value ? 0.0f : value;
			    };
			    break;
			case 2:
			    *placeValue++ = [hourDial](unsigned digit) {
				return digitize1(12, digit, 0, hourDial);
			    };
			    break;
			case 1:
			    *placeValue++ = [minuteDial](unsigned digit) {
				return digitize0(60, digit, 1, minuteDial);
			    };
			    break;

			case 0:
			    *placeValue++ = [minuteDial](unsigned digit) {
				return digitize0(60, digit, 0, minuteDial);
			    };
			    break;
			}
		    }
#endif
		    ++placeIndex;
		}
		{
		    WaveDial dial[2] {
			{inBiSecondOf	(secondsSinceTwelveLocaltime)},
			{inNearBiSecondOf	(secondsSinceTwelveLocaltime)},
		    };
		    dots = [dial](unsigned dot) {
			return dial[dot](0.0f);
		    };
		}
	    } break;
	}

	// set decimal place digits in image
	unsigned placeIndex {0};
	for (auto & pwms: pca9685Pwms) {
	    for (unsigned digit = 0; digit < 10; digit++) {
		static unsigned constexpr pwmOf[10]
		    {5, 1, 3, 10, 2, 13, 6, 11, 15, 14};
		pwms[pwmOf[digit]] = snapOffNixie(digit, PCA9685::Pwm::max
		    * fadeNixie
		    * placeValues[placeIndex](digit)
		    * bias(digit));
	    }
	    ++placeIndex;
	}

	// set colon dots in image
	PCA9685::Pwm * dotPwms[] {&pca9685Pwms[1][4], &pca9685Pwms[2][12]};
	unsigned dot {0};
	for (auto pwm: dotPwms) {
	    *pwm = snapOffNixie(10, PCA9685::Pwm::max
		* fadeNixie
		* dots(dot)
		* bias(10));
	    ++dot;
	}
    }

    // show the image

    // SPI::Transaction constructor queues the ledMessage.
    // SPI::Transaction destructor waits for result.
    // queue both before waiting for result of either.
    {
	SPI::Transaction transaction(spiDevice, SPI::Transaction::Config()
	    .tx_buffer_(&ledMessage)
	    .length_(ledMessage.length()));
    }

    for (auto & pca9685: pca9685s) {
	auto const place {&pca9685 - pca9685s.data()};
	pca9685.setPwms(0, pca9685Pwms[place], PCA9685::pwmCount, true);
    }
}

void NixieArtTask::update() {
    // if the rate at which we complete work
    // is less than the periodic demand for such work
    // the work queue will eventually exhaust all memory.
    if (updated++) {
	// catch up to the rate of demand
	ESP_LOGW(name, "update rate too fast. %u", updated - 1);
	return;
    }
    update_();
    // ignore any queued update work
    io.poll();
    updated = 0;
}

static PCA9685::Mode const pca9685Mode {
    0,	// allcall
    0,	// sub3
    0,	// sub2
    0,	// sub1
    0,	// sleep
    1,	// ai		enable autoincrement
    0,	// extclk
    0,	// restart
    0,	// outne
    1,	// outdrv	configure outputs with a totem pole structure
    0,	// och		change outputs on STOP command
    0,	// invrt	do not invert output logic state
};

static char const * const levelsKey[] {
    "aLevel",
    "bLevel",
    "cLevel",
};
static char const * const dimsKey[] {
    "aDim",
    "bDim",
    "cDim",
};
static char const * const colorsKey[] {
    "aColor",
    "bColor",
};

void NixieArtTask::levelObserved(size_t index, char const * value_) {
    float value {std::min(1.0f, (0.5f + fromString<unsigned>(value_)) / 4096.0f)};
    if (0.0f <= value && value <= 1.0f) {
	io.post([this, index, value](){
	    levels[index] = value;
	});
    }
}

void NixieArtTask::dimObserved(size_t index, char const * value_) {
    float value {std::min(1.0f, (0.5f + fromString<unsigned>(value_)) / 4096.0f)};
    if (0.0f <= value && value <= 1.0f) {
	io.post([this, index, value](){
	    dims[index] = value;
	});
    }
}

void NixieArtTask::colorObserved(size_t index, char const * value_) {
    if (APA102::isColor(value_)) {
	APA102::LED<> value(value_);
	io.post([this, index, value](){
	    colors[index] = value;
	});
    }
}

NixieArtTask::NixieArtTask(
    KeyValueBroker &		keyValueBroker)
:
    AsioTask		{"NixieArtTask", 5, 0x10000, 1},
    TimePreferences	{io, keyValueBroker, 128},

    spiBus {HSPI_HOST, SPI::Bus::Config()
	.mosi_io_num_(SPI::Bus::HspiConfig.mosi_io_num)
	.sclk_io_num_(SPI::Bus::HspiConfig.sclk_io_num),
    1},

    spiDevice {&spiBus, SPI::Device::Config()
	.mode_(APA102::spiMode)
	.clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	.spics_io_num_(-1)		// no chip select
	.queue_size_(1)
    },

    // internal pullups on silicon are rather high (~50k?)
    // external 4.7k is still too high. external 1k works
    i2cMasters {{
	{I2C::Config()
		.sda_io_num_(GPIO_NUM_4) //.sda_pullup_en_(GPIO_PULLUP_ENABLE)
		.scl_io_num_(GPIO_NUM_5) //.scl_pullup_en_(GPIO_PULLUP_ENABLE)
		.master_clk_speed_(400000),	// I2C fast mode
	    I2C_NUM_0, 0
	},
	{I2C::Config()
		.sda_io_num_(GPIO_NUM_27) //.sda_pullup_en_(GPIO_PULLUP_ENABLE)
		.scl_io_num_(GPIO_NUM_33) //.scl_pullup_en_(GPIO_PULLUP_ENABLE)
		.master_clk_speed_(200000),
	    I2C_NUM_1, 0
	}
    }},

    pca9685s {{
	{&i2cMasters[0], PCA9685::addressOf(0), pca9685Mode},
	{&i2cMasters[0], PCA9685::addressOf(1), pca9685Mode},
	{&i2cMasters[0], PCA9685::addressOf(2), pca9685Mode},
	{&i2cMasters[0], PCA9685::addressOf(3), pca9685Mode},
    }},

    sensorTask	{},
    luxSensor	{[this]() -> LuxSensor * {
	    try {
		return new TSL2591LuxSensor(sensorTask.io, &i2cMasters[1]);
	    } catch (esp_err_t & e) {
		ESP_LOGE(name, "TSL2591 %s (0x%x): disabled", esp_err_to_name(e), e);
		return nullptr;
	    } catch (...) {
		ESP_LOGE(name, "TSL2591 unknown error: disabled");
		return nullptr;
	    }
	}()
    },
    motionSensor	{[this]() -> HT7M2xxxMotionSensor * {
	    try {
		return new HT7M2xxxMotionSensor(sensorTask.io, &i2cMasters[1]);
	    } catch (esp_err_t & e) {
		ESP_LOGE(name, "HT7M2xxx %s (0x%x): disabled", esp_err_to_name(e), e);
		return nullptr;
	    } catch (...) {
		ESP_LOGE(name, "HT7M2xxx unknown error: disabled");
		return nullptr;
	    }
	}()
    },

    levels	{},
    dims	{},
    colors	{},
    black	{1.0f / (1 << 10)},
    white	{1.0f * (1 <<  4)},

    levelsObserver {
	{keyValueBroker, levelsKey[0],  "192",
	    [this](char const * value) {levelObserved(0, value);}},
	{keyValueBroker, levelsKey[1], "1024",
	    [this](char const * value) {levelObserved(1, value);}},
	{keyValueBroker, levelsKey[2], "4096",
	    [this](char const * value) {levelObserved(2, value);}},
    },

    dimsObserver {
	{keyValueBroker, dimsKey[0], "4096",
	    [this](char const * value) {dimObserved(0, value);}},
	{keyValueBroker, dimsKey[1], "4096",
	    [this](char const * value) {dimObserved(1, value);}},
	{keyValueBroker, dimsKey[2], "4032",
	    [this](char const * value) {dimObserved(2, value);}},
    },

    colorsObserver {
	{keyValueBroker, colorsKey[0], "#aa80ff",
	    [this](char const * value) {colorObserved(0, value);}},
	{keyValueBroker, colorsKey[1], "#aa80ff",
	    [this](char const * value) {colorObserved(1, value);}},
    },

    blackObserver {keyValueBroker, "black", "10",
	[this](char const * value_) {
	    unsigned const value {fromString<unsigned>(value_)};
	    io.post([this, value](){
		black = 1.0f / (1 << value);
	    });
	}
    },

    whiteObserver {keyValueBroker, "white", "4",
	[this](char const * value_) {
	    unsigned const value {fromString<unsigned>(value_)};
	    io.post([this, value](){
		white = 1.0f * (1 << value);
	    });
	}
    },

    pirgainObserver {keyValueBroker, "pirgain", "16",
	[this](char const * value_) {
	    if (motionSensor) {
		unsigned const value {fromString<unsigned>(value_)};
		sensorTask.io.post([this, value](){
		    try {
			motionSensor->setConfiguration1(
			    motionSensor->getConfiguration1().pirGain_(value));
		    } catch (esp_err_t & e) {
			ESP_LOGE(name, "motionSensor setPirGain %s (0x%x)", esp_err_to_name(e), e);
		    }
		});
	    }
	}
    },

    pirbaseObserver {keyValueBroker, "pirbase", "0",
	[this](char const * value_) {
	    if (motionSensor) {
		unsigned const value {fromString<unsigned>(value_)};
		sensorTask.io.post([this, value](){
		    try {
			motionSensor->setConfiguration1(
			    motionSensor->getConfiguration1().pirThreshold_(value));
		    } catch (esp_err_t & e) {
			ESP_LOGE(name, "motionSensor setPirThreshold %s (0x%x)", esp_err_to_name(e), e);
		    }
		});
	    }
	}
    },

    pirtimeObserver {keyValueBroker, "pirtime", "3",
	[this](char const * value_) {
	    if (motionSensor) {
		unsigned const value {fromString<unsigned>(value_)};
		sensorTask.io.post([this, value](){
		    try {
			motionSensor->setDuration(((1 << value) - 1) * 10);
		    } catch (esp_err_t & e) {
			ESP_LOGE(name, "motionSensor setDuration %s (0x%x)", esp_err_to_name(e), e);
		    }
		});
	    }
	}
    },

    microsecondsSinceBootOfModeChange(0u),

    mode(Mode::clock),
    modeObserver(keyValueBroker, "mode", mode.toString(),
	[this](char const * value){
	    Mode mode_(value);
	    io.post([this, mode_](){
		mode = mode_;
		microsecondsSinceBootOfModeChange = get_time_since_boot();
	    });
	}),

    updated(0)
{
    if (motionSensor) {
	motionSensor->setConfiguration0(motionSensor->getConfiguration0()
	    .pirPeriod_(HT7M2xxxMotionSensor::PirPeriod::_32ms));
	motionSensor->setConfiguration1(motionSensor->getConfiguration1()
	    .lowVoltageDetectEnable_	(false)
	    .pirDetectEnable_		(true)
	    .pirRetrigger_		(true)
	    .pirTriggeredPinEnable_	(false));
	motionSensor->setConfiguration2(motionSensor->getConfiguration2()
	    .pirDetectIfDarkEnable_	(false));
    }
}

void NixieArtTask::run() {
    sensorTask.start();

    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, 4, true, [this](){
	io.post([this](){
	    this->update();
	});
    });

    updateTimer.start();

    // create some dummy work ...
    asio::io_service::work work(io);

    // ... so that we will run forever
    AsioTask::run();
}

NixieArtTask::~NixieArtTask() {
    stop();
    ESP_LOGI(name, "~NixieArtTask");
}
