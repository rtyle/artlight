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

using APA102::LED;
using LEDI = APA102::LED<int>;

static float constexpr pi	= std::acos(-1.0f);
static float constexpr tau	= 2.0f * pi;

static float constexpr phi	= (1.0f + std::sqrt(5.0f)) / 2.0f;
static float constexpr sqrt2	= std::sqrt(2.0f);

static unsigned constexpr millisecondsPerSecond	= 1000u;
static unsigned constexpr microsecondsPerSecond	= 1000000u;

static Pulse hourPulse	(12);
static Pulse minutePulse(60);
static Pulse secondPulse(60);

static SawtoothCurve inSecondOf(0.0f, 1.0f);
static SawtoothCurve inMinuteOf(0.0f, 60.0f);
static SawtoothCurve inHourOf  (0.0f, 60.0f * 60.0f);
static SawtoothCurve inDayOf   (0.0f, 60.0f * 60.0f * 12.0f);	/// 12 hour clock

char const * const ClockArtTask::Mode::string[]
    {"clock", "slide", "spin"};
ClockArtTask::Mode::Mode(Value value_) : value(value_) {}
ClockArtTask::Mode::Mode(char const * value) : value(
    [value](){
	size_t i = 0;
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

void ClockArtTask::update_() {
    // one LED strip (ring) for hours, the other for minutes and seconds.
    // for rendering to accurately index these rings,
    // these numbers must reflect the arguments of the *InRing constructors (below).
    static size_t constexpr ringSize[] = {
	59 + 59 + 59 + 57 + 57 + 55 + 55 + 55 + 55 + 56 + 57 + 59,
	20 + 27 + 34 + 25 + 24 + 25 + 22 + 24 + 26 + 32 + 29 + 29
    };

    uint64_t const microsecondsSinceBoot = esp_timer_get_time();

    static LEDI const black(0, 0, 0);
    Blend<LEDI> const blend[] {
	{black, color[0]},
	{black, color[1]},
	{black, color[2]},
    };

    static float constexpr scale[] {
	phi	/ 1.5f,		//  > 1.0
	sqrt2	/ 1.5f,		//  < 1.0
	1.5f	/ 1.5f,		// == 1.0
    };

    std::list<std::function<LEDI(float)>> renderList[2];

    // construct static PerlinNoise objects
    static std::mt19937 rng;
    static PerlinNoise perlinNoise[] {rng, rng, rng, rng};
    // Perlin noise repeats every 256 units.
    static unsigned constexpr perlinNoisePeriod = 256;
    static uint64_t constexpr perlinNoisePeriodMicroseconds
	= perlinNoisePeriod * microsecondsPerSecond;
    // Perlin noise at an integral grid point is 0.
    // To avoid this, cut between them.

    switch (mode.value) {
    case Mode::Value::clock: {
	    static size_t constexpr toRingIndex[] = {0, 1, 1};

	    float widthInRing[] {
		width[0] / ringSize[toRingIndex[0]],
		width[1] / ringSize[toRingIndex[1]],
		width[2] / ringSize[toRingIndex[2]],
	    };

	    Shape shape_[3] {shape[0], shape[1], shape[2]};

	    float secondsSinceTwelveLocaltime
		= smoothTime.millisecondsSinceTwelveLocaltime(
			microsecondsSinceBoot)
		    / static_cast<float>(millisecondsPerSecond);
	    float position[3] {
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

	    static float constexpr waveWidth[] = {
		2.0f / ringSize[0],
		2.0f / ringSize[1]
	    };

	    for (size_t index = 0; index < 3; ++index) {
		size_t ringIndex = toRingIndex[index];
		if (widthInRing[index]) {
		    switch (shape_[index].value) {
		    case Shape::Value::bell: {
			    BellCurve<Dial> dial(
				position[index], widthInRing[index]);
			    renderList[ringIndex].push_back([&blend, index, dial](
				    float place){
				return blend[index](dial(place));
			    });
			}
			break;
		    case Shape::Value::wave: {
			    BellStandingWaveDial dial(position[index],
				widthInRing[index],
				phaseIn(microsecondsSinceBoot,
				    microsecondsPerSecond * 2.0f
				    / scale[index] / waveWidth[ringIndex]),
				waveWidth[ringIndex]);
			    renderList[ringIndex].push_back([&blend, index, dial](
				    float place){
				return blend[index](dial(place));
			    });
			}
			break;
		    case Shape::Value::bloom: {
			    Dial dial(position[index]);
			    BumpCurve bump(0.0f, widthInRing[index]);
			    BloomCurve bloom(0.0f, widthInRing[index],
				phaseIn(microsecondsSinceBoot,
				    microsecondsPerSecond * 2.0f
				    / scale[index]));
			    renderList[ringIndex].push_back([
				    &blend, index, dial, bump, bloom](
				    float place){
				float offset = dial(place);
				return blend[index](
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
	    float z = (microsecondsSinceBoot % perlinNoisePeriodMicroseconds)
		/ static_cast<float>(microsecondsPerSecond);
	    renderList[0].push_back([z](float place){
		static float constexpr radius = 0.5f;
		float x = radius * std::cos(tau * place);
		float y = radius * std::sin(tau * place);
		static int constexpr max = 128;
		static int constexpr octaves = 1;
		return LEDI(
		    max * perlinNoise[0].octaveNoise0_1(x, y, z, octaves),
		    max * perlinNoise[1].octaveNoise0_1(x, y, z, octaves),
		    max * perlinNoise[2].octaveNoise0_1(x, y, z, octaves));
	    });
	} break;
    case Mode::Value::spin: {
	    float x = (microsecondsSinceBoot
		    % perlinNoisePeriodMicroseconds)
		/ static_cast<float>(microsecondsPerSecond);
	    static int constexpr maxBrightness = 128;
	    static int constexpr octaves = 1;
	    LEDI color(
		maxBrightness * perlinNoise[0].octaveNoise0_1(x, 0.5f, octaves),
		maxBrightness * perlinNoise[1].octaveNoise0_1(x, 0.5f, octaves),
		maxBrightness * perlinNoise[2].octaveNoise0_1(x, 0.5f, octaves));
	    Blend<LEDI> blend(black, color);
	    static float constexpr minWidth = 8.0f;
	    static float constexpr varWidth = 8.0f;
	    // a change in the standing wave width will cause it to spin
	    float width = (minWidth + varWidth * perlinNoise[3].octaveNoise0_1(
		    ((microsecondsSinceBoot / 2u)
			    % perlinNoisePeriodMicroseconds)
			/ static_cast<float>(microsecondsPerSecond),
		    0.0f, octaves)
		) / ringSize[0];
	    unsigned period = 4u * microsecondsPerSecond / width;
	    uint64_t microsecondsSinceLastPeriod
		= microsecondsSinceBoot - microsecondsSinceBootOfLastPeriod;
	    unsigned offset = microsecondsSinceLastPeriod % period;
	    microsecondsSinceBootOfLastPeriod = microsecondsSinceBoot - offset;
	    float position = offset / static_cast<float>(period);
	    WaveDial right(position, width), left(-position, width);
	    renderList[0].push_back([blend, right, left](float place){
		return blend(right(place) + left(place) / 2.0f);
	    });
	} break;
    }

    static size_t constexpr folds = 12;
    static size_t constexpr foldedSize[folds]
	= {59, 59, 59, 57, 57, 55, 55, 55, 55, 56, 57, 59};
    static size_t constexpr unfoldedSize[folds]
	= { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
    FoldsInRing inRing0(folds, foldedSize, unfoldedSize);
    static size_t constexpr sectors = 12;
    static size_t constexpr sectorSize[sectors]
	= {20, 27, 34, 25, 24, 25, 22, 24, 26, 32, 29, 29};
    SectorsInRing inRing1(sectors, sectorSize);

    LEDI leds0[ringSize[0]];
    LEDI leds1[ringSize[1]];

    // render art from places in the ring,
    // keeping track of the largest led value by part.
    auto maxRendering = std::numeric_limits<int>::min();
    for (auto & led: leds0) {
	for (auto & place: *inRing0) {
	    for (auto & render: renderList[0]) {
		led = led + render(place);
	    }
	}
	++inRing0;
	maxRendering = std::max(maxRendering, led.max());
    }
    for (auto & led: leds1) {
	for (auto & place: *inRing1) {
	    for (auto & render: renderList[1]) {
		led = led + render(place);
	    }
	}
	++inRing1;
	maxRendering = std::max(maxRendering, led.max());
    }

    APA102::Message<ringSize[0]> message0;
    APA102::Message<ringSize[1]> message1;

    /// adjust brightness
    float dimming = dim.value == Dim::manual
	? dimLevel / 16.0f
	// automatic dimming as a function of measured ambient lux.
	// this will range from 3/16 to 16/16 with the numerator increasing by
	// 1 as the lux doubles up until 2^13 (~full daylight, indirect sun).
	// an LED value of 128 will be dimmed to 24 in complete darkness (lux 0)
	: (3.0f + std::min(13.0f, std::log2(1.0f + getLux()))) / 16.0f;
    LEDI * led;
    led = leds0;
    if (range.value == Range::clip) {
	for (auto & e: message0.encodings) {
	    e = LED<>(gammaEncode,
		clip(*led++) * dimming);
	}
    } else if (range.value == Range::normalize) {
	static auto maxEncoding = std::numeric_limits<uint8_t>::max();
	for (auto & e: message0.encodings) {
	    e = LED<>(gammaEncode,
		(*led++ * maxEncoding / maxRendering) * dimming);
	}
    }
    led = leds1;
    if (range.value == Range::clip) {
	for (auto & e: message1.encodings) {
	    e = LED<>(gammaEncode,
		clip(*led++) * dimming);
	}
    } else if (range.value == Range::normalize) {
	static auto maxEncoding = std::numeric_limits<uint8_t>::max();
	for (auto & e: message1.encodings) {
	    e = LED<>(gammaEncode,
		(*led++ * maxEncoding / maxRendering) * dimming);
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
    SPI::Bus const		(&spiBus)[2],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ArtTask		("ClockArtTask", 5, 0x10000, 1,
    			spiBus, getLux_, keyValueBroker_, 512),

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
		bool reverse_ = std::strcmp("0", value);
		io.post([this, reverse_](){
		    reverse = reverse_;
		});
	    }),

    microsecondsSinceBootOfLastPeriod(0u),

    updated(0)
{}

void ClockArtTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, 8, true, [this](){
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
