#include <sstream>

#include <esp_log.h>

#include "clip.h"
#include "fromString.h"
#include "Blend.h"
#include "CornholeArtTask.h"
#include "Dial.h"
#include "InRing.h"
#include "Pulse.h"
#include "Sawtooth.h"
#include "Timer.h"

using APA102::LED;
using LEDI = APA102::LED<int>;

static float constexpr phi	= (1.0f + std::sqrt(5.0f)) / 2.0f;
static float constexpr sqrt2	= std::sqrt(2.0f);

static auto constexpr millisecondsPerSecond	= 1000u;

static size_t constexpr ringSize = 80;

static Pulse hourPulse	(0);//12);
static Pulse minutePulse(0);//60);
static Pulse secondPulse(0);//60);

static Sawtooth inSecondOf(1.0f);
static Sawtooth inMinuteOf(60.0f);
static Sawtooth inHourOf  (60.0f * 60.0f);
static Sawtooth inDayOf   (60.0f * 60.0f * 12.0f);	/// 12 hour clock

char const * const CornholeArtTask::Mode::string[] {"score", "clock",};
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

void updateLedChannel(LEDC::Channel & ledChannel, uint32_t duty) {
    // do not use ledChannel.set_duty_and_update
    ledChannel.set_duty(duty);
    ledChannel.update_duty();
}

void updateLedChannelRGB(LEDC::Channel (&ledChannels)[3], LEDI const & color) {
    LEDC::Channel * ledChannel = ledChannels;
    for (auto part: {color.part.red, color.part.green, color.part.blue}) {
	updateLedChannel(*ledChannel++, part);
    }
}

static unsigned constexpr scoreMax = 21;

void CornholeArtTask::update() {
    uint64_t microsecondsSinceBoot = esp_timer_get_time();
    float secondsSinceBoot = microsecondsSinceBoot / 1000000.0f;

    static BumpDial dial;

    static LEDI black(0, 0, 0);
    Blend<LEDI> blend[] {
	{black, color[0]},
	{black, color[1]},
	{black, color[2]},
    };

    float place = secondsSinceBoot / 9.0f;

    LEDC::Channel (*rgb)[3] = &ledChannel[0];
    updateLedChannelRGB(*rgb++, blend[0](dial(place * phi  )));
    updateLedChannelRGB(*rgb++, blend[1](dial(place * sqrt2)));
    updateLedChannelRGB(*rgb++, blend[2](dial(place * 1.5f )));

    float widthInRing[] {
	width[0] / ringSize,
	width[1] / ringSize,
	width[2] / ringSize,
    };

    float position[3] {};
    switch (mode.value) {
    case Mode::score: {
	position[0] = score[0] / static_cast<float>(scoreMax);
	position[1] = score[1] / static_cast<float>(scoreMax);
	position[2] = position[0] - position[1];
	if (score[0] != score[1]) {
	    if (scoreMax <= score[0]) {
		widthInRing[0] *= 3.0f;
		widthInRing[1]  = 0.0f;
	    } else if (scoreMax <= score[1]) {
		widthInRing[1] *= 3.0f;
		widthInRing[0]  = 0.0f;
	    }
	}
	widthInRing[2] = 0.0f;
    } break;
    case Mode::clock: {
	float secondsSinceTwelveLocaltime
	    = smoothTime.millisecondsSinceTwelveLocaltime(microsecondsSinceBoot)
		/ millisecondsPerSecond;
	position[0] = hourPulse  (inDayOf   (secondsSinceTwelveLocaltime));
	position[1] = minutePulse(inHourOf  (secondsSinceTwelveLocaltime));
	position[2] = secondPulse(inMinuteOf(secondsSinceTwelveLocaltime));
    } break;
    }

    std::list<std::function<LEDI(float)>> renderList;

    static float constexpr waveWidth = 2.0f / ringSize;

    static float constexpr scale[] {
	phi	/ 1.5f,		//  > 1.0
	sqrt2	/ 1.5f,		//  < 1.0
	1.5f	/ 1.5f,		// == 1.0
    };

    for (size_t index = 0; index < 3; ++index) {
	if (widthInRing[index]) {
	    switch (shape[index].value) {
	    case Shape::Value::bell: {
		    BellDial dial(position[index], widthInRing[index]);
		    renderList.push_back([&blend, index, dial](float place){
			return blend[index](dial(place));
		    });
		}
		break;
	    case Shape::Value::wave: {
		    BellStandingWaveDial dial(position[index],
			widthInRing[index],
			scale[index] * secondsSinceBoot * waveWidth / 2.0f,
			waveWidth);
		    renderList.push_back([&blend, index, dial](float place){
			return blend[index](dial(place));
		    });
		}
		break;
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
	    [this](unsigned count){scoreIncrement(0, count);},
	    [this](unsigned count){scoreDecrement(0, count);},
	},
	{pin[1], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned count){scoreIncrement(1, count);},
	    [this](unsigned count){scoreDecrement(1, count);},
	},
	{pin[2], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned count){ESP_LOGI(name, "button c press %d", count);},
	    [this](unsigned count){ESP_LOGI(name, "button c hold %d" , count);}
	},
	{pin[3], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned count){ESP_LOGI(name, "button d press %d", count);},
	    [this](unsigned count){ESP_LOGI(name, "button d hold %d" , count);}
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
	})
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
