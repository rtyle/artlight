#include <random>
#include <sstream>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "clip.h"
#include "fromString.h"
#include "Blend.h"
#include "ClockArtTask.h"
#include "Curve.h"
#include "InRing.h"
#include "PerlinNoise.hpp"
#include "Pulse.h"
#include "Timer.h"

extern "C" uint64_t get_time_since_boot();

using APA102::LED;
using LEDI = APA102::LED<int>;

static float constexpr pi	{std::acos(-1.0f)};
static float constexpr tau	{2.0f * pi};

static float constexpr phi	{(1.0f + std::sqrt(5.0f)) / 2.0f};
static float constexpr sqrt2	{std::sqrt(2.0f)};

static unsigned constexpr millisecondsPerSecond	{1000u};
static unsigned constexpr microsecondsPerSecond	{1000000u};

static Pulse hourPulse	{12};
static Pulse minutePulse{60};
static Pulse secondPulse{60};

static SawtoothCurve inMinuteOf{0.0f, 60.0f};
static SawtoothCurve inHourOf  {0.0f, 60.0f * 60.0f};
static SawtoothCurve inDayOf   {0.0f, 60.0f * 60.0f * 12.0f};	/// 12 hour clock

char const * const ClockArtTask::Mode::string[]
    {"clock", "slide", "spin"};
ClockArtTask::Mode::Mode(Value value_) : value(value_) {}
ClockArtTask::Mode::Mode(char const * value) : value(
    [value](){
	size_t i {0};
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return clock;
    }()
) {}
char const * ClockArtTask::Mode::toString() const {
    return string[value];
}

static float phaseIn(uint64_t time, uint64_t period) {
    return (time % period) / static_cast<float>(period);
}

static size_t constexpr sum_(size_t const s, size_t const n, size_t const * const e) {
    return n ? sum_(s + *e, n - 1, e + 1) : s;
}
template<size_t n>
static size_t constexpr sum(size_t const (&a)[n]) {
    return sum_(0, n, a);
}

void ClockArtTask::update_() {
    static size_t constexpr dialCount	{3};
    static size_t constexpr ringCount	{2};
    static size_t constexpr sectorCount	{12};

    static size_t constexpr ring0UnfoldedSize[sectorCount]
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
    static size_t constexpr ring0FoldedSize[sectorCount]
	{59, 59, 59, 57, 57, 55, 55, 55, 55, 56, 57, 58};
    static size_t constexpr ring0SectorSize[sectorCount]
	{59, 59, 59, 57, 57, 55, 55, 55, 55, 56, 57, 58};
    static size_t constexpr ring1SectorSize[sectorCount]
	{20, 27, 34, 25, 24, 25, 22, 24, 26, 32, 29, 29};

    static size_t constexpr toRingIndex[dialCount] {0, 1, 1};

    static size_t constexpr ledCount[ringCount] {
	sum(ring0SectorSize),
	sum(ring1SectorSize)
    };

    uint64_t const microsecondsSinceBoot {get_time_since_boot()};

    static LEDI const black {0, 0, 0};
    Blend<LEDI> const blend[dialCount] {
	{black, color[0]},
	{black, color[1]},
	{black, color[2]},
    };

    static float constexpr scale[dialCount] {
	phi	/ 1.5f,		//  > 1.0
	sqrt2	/ 1.5f,		//  < 1.0
	1.5f	/ 1.5f,		// == 1.0
    };

    std::list<std::function<LEDI(float)>> renderList[ringCount];

    // construct static PerlinNoise objects
    static std::mt19937 rng;
    static PerlinNoise perlinNoise[] {rng, rng, rng, rng};
    // Perlin noise repeats every 256 units.
    static unsigned constexpr perlinNoisePeriod {256};
    static uint64_t constexpr perlinNoisePeriodMicroseconds
	{perlinNoisePeriod * microsecondsPerSecond};
    // Perlin noise at an integral grid point is 0.
    // To avoid this, cut between them.

    switch (mode.value) {
    case Mode::Value::clock: {
	    float widthInRing[dialCount] {
		width[0] / ledCount[toRingIndex[0]],
		width[1] / ledCount[toRingIndex[1]],
		width[2] / ledCount[toRingIndex[2]],
	    };

	    Shape shape_[dialCount] {shape[0], shape[1], shape[2]};

	    float const secondsSinceTwelveLocaltime
		{smoothTime.millisecondsSinceTwelveLocaltime(
			microsecondsSinceBoot)
		    / static_cast<float>(millisecondsPerSecond)};
	    float position[dialCount] {
		position[0] = hourPulse  (inDayOf   (
		    secondsSinceTwelveLocaltime)),
		position[1] = minutePulse(inHourOf  (
		    secondsSinceTwelveLocaltime)),
		position[2] = secondPulse(inMinuteOf(
		    secondsSinceTwelveLocaltime))
	    };
	    if (reverse) {
		for (float & p: position) {
		    if (0.0f < p) p = 1.0f - p;
		}
	    }

	    static float constexpr waveWidth[ringCount] {
		2.0f / ledCount[0],
		2.0f / ledCount[1]
	    };

	    for (size_t dialIndex {0}; dialIndex < dialCount; ++dialIndex) {
		size_t const ringIndex {toRingIndex[dialIndex]};
		if (widthInRing[dialIndex]) {
		    switch (shape_[dialIndex].value) {
		    case Shape::Value::bell: {
			    BellCurve<Dial> dial(
				position[dialIndex], widthInRing[dialIndex]);
			    renderList[ringIndex].push_back([&blend, dialIndex, dial](
				    float place){
				return blend[dialIndex](dial(place));
			    });
			}
			break;
		    case Shape::Value::wave: {
			    BellStandingWaveDial dial(position[dialIndex],
				widthInRing[dialIndex],
				phaseIn(microsecondsSinceBoot,
				    microsecondsPerSecond * 2.0f
				    / scale[dialIndex] / waveWidth[ringIndex]),
				waveWidth[ringIndex]);
			    renderList[ringIndex].push_back([&blend, dialIndex, dial](
				    float place){
				return blend[dialIndex](dial(place));
			    });
			}
			break;
		    case Shape::Value::bloom: {
			    Dial dial(position[dialIndex]);
			    BumpCurve bump(0.0f, widthInRing[dialIndex]);
			    BloomCurve bloom(0.0f, widthInRing[dialIndex],
				phaseIn(microsecondsSinceBoot,
				    microsecondsPerSecond * 2.0f
				    / scale[dialIndex]));
			    renderList[ringIndex].push_back([
				    &blend, dialIndex, dial, bump, bloom](
				    float place){
				float offset {dial(place)};
				return blend[dialIndex](
				    bump(offset) * bloom(offset));
			    });
			}
			break;
		    }
		}
	    }
	} break;
    case Mode::Value::slide: {
	    // cut RGB cylinders through Perlin noise space/time.
	    float z {(microsecondsSinceBoot % perlinNoisePeriodMicroseconds)
		/ static_cast<float>(microsecondsPerSecond)};
	    renderList[0].push_back([z](float place){
		static float constexpr radius {0.5f};
		float x {radius * std::cos(tau * place)};
		float y {radius * std::sin(tau * place)};
		static int constexpr max {128};
		static int constexpr octaves {1};
		return LEDI(
		    max * perlinNoise[0].octaveNoise0_1(x, y, z, octaves),
		    max * perlinNoise[1].octaveNoise0_1(x, y, z, octaves),
		    max * perlinNoise[2].octaveNoise0_1(x, y, z, octaves));
	    });
	} break;
    case Mode::Value::spin: {
	    float x {(microsecondsSinceBoot
		    % perlinNoisePeriodMicroseconds)
		/ static_cast<float>(microsecondsPerSecond)};
	    static int constexpr maxBrightness {128};
	    static int constexpr octaves {1};
	    LEDI color(
		maxBrightness * perlinNoise[0].octaveNoise0_1(x, 0.5f, octaves),
		maxBrightness * perlinNoise[1].octaveNoise0_1(x, 0.5f, octaves),
		maxBrightness * perlinNoise[2].octaveNoise0_1(x, 0.5f, octaves));
	    Blend<LEDI> blend(black, color);
	    static float constexpr minWidth {8.0f};
	    static float constexpr varWidth {8.0f};
	    // a change in the standing wave width will cause it to spin
	    float width {(minWidth + varWidth * perlinNoise[3].octaveNoise0_1(
		    ((microsecondsSinceBoot / 2u)
			    % perlinNoisePeriodMicroseconds)
			/ static_cast<float>(microsecondsPerSecond),
		    0.0f, octaves)
		) / ledCount[0]};
	    unsigned period {static_cast<unsigned>(4u * microsecondsPerSecond / width)};
	    uint64_t microsecondsSinceLastPeriod
		{microsecondsSinceBoot - microsecondsSinceBootOfLastPeriod};
	    unsigned offset {static_cast<unsigned>(microsecondsSinceLastPeriod % period)};
	    microsecondsSinceBootOfLastPeriod = microsecondsSinceBoot - offset;
	    float position {offset / static_cast<float>(period)};
	    WaveDial right(position, width), left(-position, width);
	    renderList[0].push_back([blend, right, left](float place){
		return blend(right(place) + left(place) / 2.0f);
	    });
	} break;
    }

    FoldsInRing foldsInRing0(sectorCount, ring0FoldedSize, ring0UnfoldedSize);
    SectorsInRing sectorsInRing0(sectorCount, ring0SectorSize);
    SectorsInRing sectorsInRing1(sectorCount, ring1SectorSize);

    InRing * const inRing[ringCount] {
	Mode::Value::clock == mode.value
	    ? static_cast<InRing *>(&foldsInRing0)
	    : static_cast<InRing *>(&sectorsInRing0),
	&sectorsInRing1
    };

    // render art from places in the ring,
    // keeping track of the largest led value by part.
    auto maxRendering {std::numeric_limits<int>::min()};
    LEDI leds[sum(ledCount)];
    LEDI * led;
    led = leds;
    for (size_t ringIndex {0}; ringIndex < ringCount; ++ringIndex) {
	for (size_t ledIndex {0}; ledIndex < ledCount[ringIndex]; ++ledIndex, ++led) {
	    for (auto & place: **inRing[ringIndex]) {
		for (auto & render: renderList[ringIndex]) {
		    *led = *led + render(place);
		}
	    }
	    ++*inRing[ringIndex];
	    maxRendering = std::max(maxRendering, led->max());
	}
    }

    APA102::Message<ledCount[0]> message0;
    APA102::Message<ledCount[1]> message1;

    /// adjust brightness
    float dimming {dim.value == Dim::manual
	? dimLevel / 16.0f
	// automatic dimming as a function of measured ambient lux.
	// this will range from 3/16 to 16/16 with the numerator increasing by
	// 1 as the lux doubles up until 2^13 (~full daylight, indirect sun).
	// an LED value of 128 will be dimmed to 24 in complete darkness (lux 0)
	: (3.0f + std::min(13.0f, std::log2(1.0f + luxTask.getLux()))) / 16.0f};
    led = leds;
    uint32_t * const encodings[ringCount] = {
	message0.encodings,
	message1.encodings
    };
    for (size_t ringIndex {0}; ringIndex < ringCount; ++ringIndex) {
	uint32_t * e {encodings[ringIndex]};
	for (size_t ledIndex {0}; ledIndex < ledCount[ringIndex]; ++ledIndex, ++led) {
	    static auto const maxEncoding {std::numeric_limits<uint8_t>::max()};
	    *e++ = LED<>(gammaEncode,
		Range::clip == range.value
		    ? clip(*led) * dimming
		    : static_cast<LED<>>((*led * maxEncoding / maxRendering)
			* dimming));
	}
    }

    // SPI::Transaction constructor queues the message.
    // SPI::Transaction destructor waits for result.
    // queue both before waiting for result of either.
    {
	SPI::Transaction transaction0(spiDevice[0], SPI::Transaction::Config()
	    .tx_buffer_(&message0)
	    .length_(message0.length()));
	SPI::Transaction transaction1(spiDevice[1], SPI::Transaction::Config()
	    .tx_buffer_(&message1)
	    .length_(message1.length()));
    }
}

void ClockArtTask::update() {
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

ClockArtTask::ClockArtTask(
    KeyValueBroker &		keyValueBroker_)
:
    DialArtTask		("ClockArtTask", 5, 0x10000, 1,
    			keyValueBroker_, 512),


    spiBus {
	{HSPI_HOST, SPI::Bus::Config()
	    .mosi_io_num_(SPI::Bus::HspiConfig.mosi_io_num)
	    .sclk_io_num_(SPI::Bus::HspiConfig.sclk_io_num),
	1},
	{VSPI_HOST, SPI::Bus::Config()
	    .mosi_io_num_(SPI::Bus::VspiConfig.mosi_io_num)
	    .sclk_io_num_(SPI::Bus::VspiConfig.sclk_io_num),
	2},
    },

    spiDevice {
	{&spiBus[0], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)			// no chip select
	    .queue_size_(1)
	},
	{&spiBus[1], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)			// no chip select
	    .queue_size_(1)
	},
    },

    // internal pullups on silicon are rather high (~50k?)
    // external 4.7k is still too high. external 1k works
    i2cMaster {I2C::Config()
	    .sda_io_num_(GPIO_NUM_21) //.sda_pullup_en_(GPIO_PULLUP_ENABLE)
	    .scl_io_num_(GPIO_NUM_22) //.scl_pullup_en_(GPIO_PULLUP_ENABLE)
	    .master_clk_speed_(400000),	// I2C fast mode
	I2C_NUM_0, 0},

    luxSensor{&i2cMaster},
    luxTask {luxSensor},

    mode(Mode::clock),
    modeObserver(keyValueBroker, "mode", mode.toString(),
	[this](char const * value){
	    Mode mode_(value);
	    io.post([this, mode_](){
		mode = mode_;
	    });
	}),

	reverse(false),
	reverseObserver(keyValueBroker, "reverse", "0",
	    [this](char const * value){
		bool reverse_ {std::strcmp("0", value)};
		io.post([this, reverse_](){
		    reverse = reverse_;
		});
	    }),

    microsecondsSinceBootOfLastPeriod(0u),

    updated(0)
{
    luxTask.start();
}

void ClockArtTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, 12, true, [this](){
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

ClockArtTask::~ClockArtTask() {
    stop();
    ESP_LOGI(name, "~ClockArtTask");
}
