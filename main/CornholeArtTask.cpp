#include <random>
#include <sstream>

#include <esp_log.h>

#include "clip.h"
#include "fromString.h"
#include "Blend.h"
#include "CornholeArtTask.h"
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

static size_t constexpr ringSize = 80;

static Pulse hourPulse	(0);//12);
static Pulse minutePulse(0);//60);
static Pulse secondPulse(0);//60);

static SawtoothCurve inSecondOf(0.0f, 1.0f);
static SawtoothCurve inMinuteOf(0.0f, 60.0f);
static SawtoothCurve inHourOf  (0.0f, 60.0f * 60.0f);
static SawtoothCurve inDayOf   (0.0f, 60.0f * 60.0f * 12.0f);	/// 12 hour clock

char const * const CornholeArtTask::Mode::string[]
    {"score", "clock", "slide", "spin"};
CornholeArtTask::Mode::Mode(Value value_) : value(value_) {}
CornholeArtTask::Mode::Mode(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return score;
    }()
) {}
char const * CornholeArtTask::Mode::toString() const {
    return string[value];
}

static void updateLedChannel(LEDC::Channel & ledChannel, uint32_t duty) {
    // do not use ledChannel.set_duty_and_update
    ledChannel.set_duty(duty);
    ledChannel.update_duty();
}

static void updateLedChannelRGB(LEDC::Channel (&ledChannels)[3], LEDI const & color) {
    LEDC::Channel * ledChannel = ledChannels;
    for (auto part: {color.part.red, color.part.green, color.part.blue}) {
	updateLedChannel(*ledChannel++, part);
    }
}

static float phaseIn(uint64_t time, uint64_t period) {
    return (time % period) / static_cast<float>(period);
}

static unsigned constexpr scoreMax = 21;

void CornholeArtTask::update() {
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

    static BumpCurve const bump(0.5);
    for (auto i = 0; i < 3; i++) {
	updateLedChannelRGB(ledChannel[i], blend[i](bump(phaseIn(
	    microsecondsSinceBoot, 6 * scale[i] * microsecondsPerSecond))));
    }

    std::list<std::function<LEDI(float)>> renderList;

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
    case Mode::Value::score:
    case Mode::Value::clock: {
	    float widthInRing[] {
		width[0] / ringSize,
		width[1] / ringSize,
		width[2] / ringSize,
	    };

	    Shape shape_[3] {shape[0], shape[1], shape[2]};
	    float position[3] {};
	    switch (mode.value) {
	    case Mode::score: {
		    position[0] = score[0] / static_cast<float>(scoreMax);
		    position[1] = score[1] / static_cast<float>(scoreMax);
		    position[2] = position[0] - position[1];
		    if (score[0] != score[1]) {
			if (scoreMax <= score[0]) {
			    widthInRing[1]  = 0.0f;
			    widthInRing[0]  = 1.0f;
			    shape_[0] = Shape::Value::bloom;
			} else if (scoreMax <= score[1]) {
			    widthInRing[0]  = 0.0f;
			    widthInRing[1]  = 1.0f;
			    shape_[1] = Shape::Value::bloom;
			}
		    }
		    widthInRing[2] = 0.0f;
		} break;
	    case Mode::clock: {
		    float secondsSinceTwelveLocaltime
			= smoothTime.millisecondsSinceTwelveLocaltime(
				microsecondsSinceBoot)
			    / millisecondsPerSecond;
		    position[0] = hourPulse  (inDayOf   (
			secondsSinceTwelveLocaltime));
		    position[1] = minutePulse(inHourOf  (
			secondsSinceTwelveLocaltime));
		    position[2] = secondPulse(inMinuteOf(
			secondsSinceTwelveLocaltime));
		} break;
	    case Mode::slide:
	    case Mode::spin:
		break;
	    }

	    static float constexpr waveWidth = 2.0f / ringSize;

	    for (size_t index = 0; index < 3; ++index) {
		if (widthInRing[index]) {
		    switch (shape_[index].value) {
		    case Shape::Value::bell: {
			    BellCurve<Dial> dial(
				position[index], widthInRing[index]);
			    renderList.push_back([&blend, index, dial](
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
				    / scale[index] / waveWidth),
				waveWidth);
			    renderList.push_back([&blend, index, dial](
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
			    renderList.push_back([
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
	    renderList.push_back([z](float place){
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
	    float w = (8.0f + 8.0f * perlinNoise[3].octaveNoise0_1(
		    ((microsecondsSinceBoot / 2u)
			    % perlinNoisePeriodMicroseconds)
			/ static_cast<float>(microsecondsPerSecond),
		    0.0f, octaves)
		) / ringSize;
	    unsigned period = 4u * microsecondsPerSecond / w;
	    uint64_t microsecondsSinceLastPeriod
		= microsecondsSinceBoot - microsecondsSinceBootOfLastPeriod;
	    unsigned offset = microsecondsSinceLastPeriod % period;
	    microsecondsSinceBootOfLastPeriod = microsecondsSinceBoot - offset;
	    float position = offset / static_cast<float>(period);
	    WaveDial right(position, w), left(-position, w);
	    renderList.push_back([blend, right, left](float place){
		return blend(right(place) + left(place) / 2.0f);
	    });
	} break;
    }

    static LEDI white(255, 255, 255);
    static Blend<LEDI> greyBlend(black, white);

    float secondsSinceHoleEvent
	= (microsecondsSinceBoot - microsecondsSinceBootOfHoleEvent)
	    / static_cast<float>(microsecondsPerSecond);
    if (8.0f > secondsSinceHoleEvent) {
	float dialInTime = BumpCurve(0.0f, 16.0f)(secondsSinceHoleEvent);
	Dial dialInSpace(0.0f);
	BumpCurve bump(0.0f, 1.0f);
	BloomCurve bloom(0.0f, 1.0f, 0.5 + secondsSinceHoleEvent / 2.0f);
	renderList.push_back(
	    [dialInTime, dialInSpace, bump, bloom](float place){
		float offset = dialInSpace(place);
		return greyBlend(dialInTime * bump(offset) * bloom(offset));
	    }
	);
    } else {
	float secondsSinceBoardEvent
	    = (microsecondsSinceBoot - microsecondsSinceBootOfBoardEvent)
		/ static_cast<float>(microsecondsPerSecond);
	if (4.0f > secondsSinceBoardEvent) {
	    // seed a random number generator with microsecondsSinceBootOfBoardEvent
	    // to play out the animation until a board event occurs at another time
	    std::mt19937 generator(microsecondsSinceBootOfBoardEvent);
	    std::uniform_real_distribution<float> distribute;
	    float nextPosition = distribute(generator);
	    for (unsigned i = 8; i--;) {
		float dialInTime = RippleCurve<>(0.0f, 1.0f / 4.0f)(
		    secondsSinceBoardEvent - 2.0f * distribute(generator));
		RippleCurve<Dial> dialInSpace(nextPosition, 1.0f / 8.0f);
		nextPosition += phi;
		renderList.push_back(
		    [dialInTime, dialInSpace](float place){
			return greyBlend(dialInTime * dialInSpace(place));
		    }
		);
	    }
	}
    }

    OrdinalsInRing inRing(ringSize);

    LEDI leds[ringSize];

    // render art from places in the ring,
    // keeping track of the largest led value by part.
    auto maxRendering = std::numeric_limits<int>::min();
    for (auto & led: leds) {
	for (auto & place: *inRing) {
	    for (auto & render: renderList) {
		led = led + render(place);
	    }
	}
	++inRing;
	maxRendering = std::max(maxRendering, led.max());
    }

    APA102::Message<ringSize> message;

    /// adjust brightness
    float dimming = dim.value == Dim::manual
	? dimLevel / 16.0f
	// automatic dimming as a function of measured ambient lux.
	// this will range from 3/16 to 16/16 with the numerator increasing by
	// 1 as the lux doubles up until 2^13 (~full daylight, indirect sun).
	// an LED value of 128 will be dimmed to 24 in complete darkness (lux 0)
	: (3.0f + std::min(13.0f, std::log2(1.0f + getLux()))) / 16.0f;
    LEDI * led = leds;
    if (range.value == Range::clip) {
	for (auto & e: message.encodings) {
	    e = LED<>(gammaEncode,
		clip(*led++) * dimming);
	}
    } else if (range.value == Range::normalize) {
	static auto maxEncoding = std::numeric_limits<uint8_t>::max();
	for (auto & e: message.encodings) {
	    e = LED<>(gammaEncode,
		(*led++ * maxEncoding / maxRendering) * dimming);
	}
    }

    SPI::Transaction transaction1(spiDevice[1], SPI::Transaction::Config()
	.tx_buffer_(&message)
	.length_(message.length()));
}

void CornholeArtTask::boardEvent() {
    microsecondsSinceBootOfBoardEvent = esp_timer_get_time();
}

void CornholeArtTask::holeEvent() {
    microsecondsSinceBootOfHoleEvent = esp_timer_get_time();
}

static char const * const scoreKey[] = {"aScore", "bScore"};

void CornholeArtTask::scoreIncrement(size_t index, unsigned count) {
    ESP_LOGI(name, "scoreIncrement %d %d", index, count);
    io.post([this, index, count](){
	// increment the score but not above max
	unsigned value = std::min(scoreMax, score[index] + 1 + count);
	std::ostringstream os;
	os << value;
	keyValueBroker.publish(scoreKey[index], os.str().c_str());
    });

}

void CornholeArtTask::scoreDecrement(size_t index, int count) {
    ESP_LOGI(name, "scoreDecrement %d %d", index, count);
    io.post([this, index, count](){
	if (0 > count) return;	// button hold released
	// decrement the score but not below 0
	// if the other scoring button is held too decrement all the way to 0
	unsigned value = button[1 ^ index].isDown()
	    ? 0
	    : std::max(0, static_cast<int>(score[index] - 1));
	std::ostringstream os;
	os << value;
	keyValueBroker.publish(scoreKey[index], os.str().c_str());
    });
}

void CornholeArtTask::scoreObserved(size_t index, char const * value_) {
    ESP_LOGI(name, "scoreObserved %d %s", index, value_);
    unsigned value = fromString<unsigned>(value_);
    if (value <= scoreMax) {
	io.post([this, index, value](){
	    score[index] = value;
	});
    }
}

static unsigned constexpr bounceDuration	=  25000;
static unsigned constexpr bufferDuration	= 150000;
static unsigned constexpr holdDuration		= 500000;

CornholeArtTask::CornholeArtTask(
    SPI::Bus const		(&spiBus)[2],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ArtTask		("CornholeArtTask", 5, 16384, 1,
    			spiBus, getLux_, keyValueBroker_),

    pinISR(),
    pinTask("pinTask", 5, 4096, tskNO_AFFINITY, 128),
    pin {
	{GPIO_NUM_5 , GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask},
	{GPIO_NUM_36, GPIO_MODE_INPUT, GPIO_PULLUP_DISABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE, pinTask},
	{GPIO_NUM_15, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask},
	{GPIO_NUM_34, GPIO_MODE_INPUT, GPIO_PULLUP_DISABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask},
    },

    button {
	{pin[0], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned	count){scoreIncrement(0, count);},
	    [this](int		count){scoreDecrement(0, count);},
	},
	{pin[1], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned	count){scoreIncrement(1, count);},
	    [this](int		count){scoreDecrement(1, count);},
	},
	{pin[2], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned	count){boardEvent();},
	    [this](int		count){if (0 > count) boardEvent();}
	},
	{pin[3], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned	count){holeEvent();},
	    [this](int		count){if (0 > count) holeEvent();}
	},
    },

    ledTimerHighSpeed(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_8_BIT),
    ledTimerLowSpeed (LEDC_LOW_SPEED_MODE , LEDC_TIMER_8_BIT),
    ledChannel {
	{
	    {ledTimerHighSpeed, GPIO_NUM_19, 255},
	    {ledTimerHighSpeed, GPIO_NUM_16, 255},
	    {ledTimerHighSpeed, GPIO_NUM_17, 255},
	},
	{
	    {ledTimerHighSpeed, GPIO_NUM_4 , 255},
	    {ledTimerHighSpeed, GPIO_NUM_25, 255},
	    {ledTimerHighSpeed, GPIO_NUM_26, 255},
	},
	{
	    {ledTimerLowSpeed, GPIO_NUM_33, 255},
	    {ledTimerLowSpeed, GPIO_NUM_27, 255},
	    {ledTimerLowSpeed, GPIO_NUM_12, 255},
	},
    },

    score {0, 0},
    scoreObserver {
	{keyValueBroker, scoreKey[0], "0",
	    [this](char const * value){scoreObserved(0, value);}},
	{keyValueBroker, scoreKey[1], "0",
	    [this](char const * value){scoreObserved(1, value);}},
    },

    mode(Mode::score),
    modeObserver(keyValueBroker, "mode", mode.toString(),
	[this](char const * value){
	    Mode mode_(value);
	    io.post([this, mode_](){
		mode = mode_;
	    });
	}),

	microsecondsSinceBootOfBoardEvent(0u),
	microsecondsSinceBootOfHoleEvent(0u),
	microsecondsSinceBootOfLastPeriod(0u)
{
    pinTask.start();
}

void CornholeArtTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, 1, true, [this](){
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

void CornholeArtTask::start() {
    // https://www.espressif.com/sites/default/files/documentation/
    //		eco_and_workarounds_for_bugs_in_esp32_en.pdf
    //	3.11. When certain RTC peripherals are powered on, the inputs of
    //		GPIO36 and GPIO39 will be pulled down for approximately 80 ns.
    //	Details:
    //		Powering on the following RTC peripherals will trigger this issue:
    //		* SARADC1
    //		* SARADC2
    //		* AMP
    //		* HALL
    //	Workarounds:
    //		When enabling power for any of these peripherals,
    //		ignore input from GPIO36 and GPIO39.
    //
    // Unfortunately, we are using GPIO36 (pin[1]) and, we assume,
    // SARADC2 (Successive Approximation Register Analog to Digital Converter 2)
    // has been powered on to support WI-FI.
    // To workaround this bug, pin[1] was constructed with interrupts disabled.
    // We assume that we are past the 80ns pull-down and enable interrupts now.
    // We must be on the same CPU that the GPIO ISR service was installed
    // on (our pinISR was (we were) constructed on).
    ObservablePin &pin36(pin[1]);
    pin36.set_intr_type(GPIO_INTR_ANYEDGE);
    pin36.intr_enable();
    AsioTask::start();
}

CornholeArtTask::~CornholeArtTask() {
    stop();
    ESP_LOGI(name, "~CornholeArtTask");
}
