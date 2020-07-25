#include <cmath>
#include <limits>
#include <numeric>

#include <esp_log.h>

#include "APA102.h"
#include "Curve.h"
#include "NixieArtTask.h"
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
    {"clock", "count", "roll"};
NixieArtTask::Mode::Mode(Value value_) : value(value_) {}
NixieArtTask::Mode::Mode(char const * value) : value(
    [value](){
	size_t i = 0;
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

void NixieArtTask::update_() {
    APA102::Message<8> message;
    for (auto & e: message.encodings) {
	e = LED<>(8, 8, 8);
    }

    // SPI::Transaction constructor queues the message.
    // SPI::Transaction destructor waits for result.
    // queue both before waiting for result of either.
    {
	SPI::Transaction transaction(spiDevice, SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
    }

    uint64_t const microsecondsSinceBoot {get_time_since_boot()};

    std::function<float(unsigned)> digits[pca9685s.size()] {
	[](unsigned) {return 0.0f;},
	[](unsigned) {return 0.0f;},
	[](unsigned) {return 0.0f;},
	[](unsigned) {return 0.0f;},
    };
    std::function<float(unsigned)> dots {
	[](unsigned) {return 0.0f;}
    };

    switch (mode.value) {
    case Mode::count : {
	    std::function<float(unsigned)> * digit = digits + pca9685s.size();
	    float /* deciseconds */ sinceModeChange {
		(microsecondsSinceBoot - microsecondsSinceBootOfModeChange)
		/ 100000.0f
	    };
	    unsigned order {4};
	    for (unsigned place = pca9685s.size(); place--;) {
		MesaDial dial {inDigitsOf(sinceModeChange), 1.0f / 10.0f, order};
		*--digit = [dial](unsigned digit) {
		    return digitize0(10, digit, 0, dial);
		};
		sinceModeChange /= 10.0f;
		order *= 10;
	    }
	} break;
	case Mode::roll : {
	    std::function<float(unsigned)> * digit = digits + pca9685s.size();
	    float /* seconds */ sinceModeChange {
		(microsecondsSinceBoot - microsecondsSinceBootOfModeChange)
		/ 1000000.0f
	    };
	    unsigned order {4};
	    for (unsigned place = pca9685s.size(); place--;) {
		MesaDial dial {inDigitsOf(sinceModeChange), 1.0f / 10.0f, order};
		*--digit = [dial](unsigned digit) {
		    return digitize0(10, digit, 0, dial);
		};
	    }
	} break;
    case Mode::clock :
    default : {
	    std::function<float(unsigned)> * digit = digits + pca9685s.size();
	    float const secondsSinceTwelveLocaltime
		{smoothTime.millisecondsSinceTwelveLocaltime(
			microsecondsSinceBoot)
		    / static_cast<float>(millisecondsPerSecond)};
	    {
		MesaDial dial
		    {inDayOf(secondsSinceTwelveLocaltime), 1.0f / 12.0f, 4 * 60 * 60};
		*--digit = [dial](unsigned digit) {
		    auto value = digitize1(12, digit, 1, dial);
		    return 0 == digit && 0.0f < value ? 0.0f : value;
		};
		*--digit = [dial](unsigned digit) {
		    return digitize1(12, digit, 0, dial);
		};
	    }
	    {
		MesaDial dial
		    {inHourOf(secondsSinceTwelveLocaltime), 1.0f / 60.0f, 4 * 60};
		*--digit = [dial](unsigned digit) {
		    return digitize0(60, digit, 1, dial);
		};
		*--digit = [dial](unsigned digit) {
		    return digitize0(60, digit, 0, dial);
		};
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

    // we will set all of a PCA9685's Pwm values at once with setPwms (below).
    // create an image of these values for each PCA9685 here
    PCA9685::Pwm pca9685Pwms[pca9685s.size()][PCA9685::pwmCount];
    static unsigned constexpr max {0xfff};
    static unsigned constexpr min {0x005};

    // set digits in image
    unsigned place = 0;
    for (auto & pwms: pca9685Pwms) {
	for (unsigned digit = 0; digit < 10; digit++) {
	    unsigned value = max * digits[place](digit);
	    if (min <= value) {
		static unsigned constexpr pwmOf[10]
		    {5, 1, 3, 10, 2, 13, 6, 11, 15, 14};
		auto & pwm {pwms[pwmOf[digit]]};
		pwm.offFull = 0;
		pwm.off = value;
	    }
	}
	++place;
    }

    // set dots in colon in image
    PCA9685::Pwm * pwms[] {&pca9685Pwms[1][4], &pca9685Pwms[2][12]};
    unsigned dot = 0;
    for (auto pwm: pwms) {
	unsigned value = max * dots(dot);
	if (min <= value) {
	    pwm->offFull = 0;
	    pwm->off = value;
	}
	++dot;
    }

    // set the image
    place = 0;
    for (auto & pca9685: pca9685s) {
	pca9685.setPwms(0, pca9685Pwms[place], PCA9685::pwmCount, true);
	++place;
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

static PCA9685::Mode pca9685Mode {
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

static char const * const levelKey[] = {
    "aLevel",
    "bLevel",
};
static char const * const colorKey[] {
    "aColor",
    "bColor",
};

void NixieArtTask::levelObserved(size_t index, char const * value_) {
    float value = fromString<float>(value_);
    if (0.0f <= value && value <= 64.0f) {
	io.post([this, index, value](){
	    level[index] = value;
	});
    }
}

void NixieArtTask::colorObserved(size_t index, char const * value_) {
    if (APA102::isColor(value_)) {
	APA102::LED<> value(value_);
	io.post([this, index, value](){
	    color[index] = value;
	});
    }
}

NixieArtTask::NixieArtTask(
    KeyValueBroker &		keyValueBroker_)
:
    ArtTask		{"NixieArtTask", 5, 0x10000, 1,
			keyValueBroker_, 128},

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
		.master_clk_speed_(200000),	// I2C fast mode
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
//    tsl2591LuxSensor	{sensorTask, &i2cMasters[1]},
    //motionSensor	{sensorTask, &i2cMasters[1]},


    level {},
    color {},

    levelObserver {
	{keyValueBroker, levelKey[0], "8",
	    [this](char const * value) {levelObserved(0, value);}},
	{keyValueBroker, levelKey[1], "8",
	    [this](char const * value) {levelObserved(1, value);}},
    },

    colorObserver {
	{keyValueBroker, colorKey[0], "#ff0000",
	    [this](char const * value) {colorObserved(0, value);}},
	{keyValueBroker, colorKey[1], "#0000ff",
	    [this](char const * value) {colorObserved(1, value);}},
    },

    microsecondsSinceBootOfModeChange(0u),

    mode(Mode::clock),
    modeObserver(keyValueBroker, "mode", mode.toString(),
	[this](char const * value){
	    Mode mode_(value);
	    io.post([this, mode_](){
		mode = mode_;
		microsecondsSinceBootOfModeChange = esp_timer_get_time();
	    });
	}),

    updated(0)
{
    // the default PCA9685 prescale value (30)
    // combined with its internal oscillator (25 MHz)
    // results in a 200 Hz update rate for its 4096 PWM values.
    //	30 = std::round(25000000 / (4096 * 200)) - 1
    // This results in a minimum PWM on time of 1.22 microseconds.
    // https://www.youtube.com/watch?v=TK3E55fytC0
    // suggests that a "warm" nixie tube can operate at 100000 Hz
    // (on time of 10 microseconds with a turn on time of less than 5).
    // it would take an update rate of 48.83 Hz to to guarantee
    // that the minimal PWM value (1) would turn on a nixie tube.
    // such a low update rate, however, would likely cause flickering.
    // it is better to keep the default 200 Hz update rate and avoid
    // sustained PWM values of less than 5 (5 * 1.22 = 6.1).
    sensorTask.start();
}

void NixieArtTask::run() {
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
