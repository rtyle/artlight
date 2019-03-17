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

static float constexpr pi	= std::acos(-1.0f);
static float constexpr tau	= 2.0f * pi;
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

static unsigned constexpr maxScore = 21;

void CornholeArtTask::update() {
    uint64_t microsecondsSinceBoot = esp_timer_get_time();
    float secondsSinceBoot = microsecondsSinceBoot / 1000000.0f;

    static BumpDial dial;

    Blend<LEDI> aBlend {LEDI(aFades), LEDI(aColor)};
    Blend<LEDI> bBlend {LEDI(bFades), LEDI(bColor)};
    Blend<LEDI> cBlend {LEDI(cFades), LEDI(cColor)};

    float place = secondsSinceBoot / 9.0f;

    LEDC::Channel (*rgb)[3] = &ledChannel[0];
    updateLedChannelRGB(*rgb++, aBlend(dial(place * phi  )));
    updateLedChannelRGB(*rgb++, bBlend(dial(place * sqrt2)));
    updateLedChannelRGB(*rgb++, cBlend(dial(place * 1.5f )));

    float aWidthInRing = aWidth / ringSize;
    float bWidthInRing = bWidth / ringSize;
    float cWidthInRing = cWidth / ringSize;

    float aPosition;
    float bPosition;
    float cPosition;
    if (score[0] || score[1]) {
	aPosition = score[0] / static_cast<float>(maxScore);
	bPosition = score[1] / static_cast<float>(maxScore);
	cPosition = aPosition - bPosition;
	if (maxScore <= score[0]) aWidthInRing = 3.0f * aWidth / ringSize;
	if (maxScore <= score[1]) bWidthInRing = 3.0f * bWidth / ringSize;
	cWidthInRing = 0.0f;
    } else {
	float secondsSinceTwelveLocaltime
	    = smoothTime.millisecondsSinceTwelveLocaltime(microsecondsSinceBoot)
		/ millisecondsPerSecond;
	aPosition = hourPulse(inDayOf(secondsSinceTwelveLocaltime));
	bPosition = minutePulse(inHourOf(secondsSinceTwelveLocaltime));
	cPosition = secondPulse(inMinuteOf(secondsSinceTwelveLocaltime));
    }

    std::list<std::function<LEDI(float)>> renderList;

    static float waveWidth = 2.0f / ringSize;

    if (aWidthInRing) {
	switch (aShape.value) {
	case Shape::Value::bell: {
		BellDial shape(aPosition, aWidthInRing);
		renderList.push_back([aBlend, shape](float place){
		    return aBlend(shape(place));
		});
	    }
	    break;
	case Shape::Value::wave: {
		BellStandingWaveDial shape(aPosition, aWidthInRing, (phi / 1.5f) *
		    secondsSinceBoot * waveWidth / 2.0f, waveWidth);
		renderList.push_back([aBlend, shape](float place){
		    return aBlend(shape(place));
		});
	    }
	    break;
	}
    }
    if (bWidthInRing) {
	switch (bShape.value) {
	case Shape::Value::bell: {
		BellDial shape(bPosition, bWidthInRing);
		renderList.push_back([bBlend, shape](float place){
		    return bBlend(shape(place));
		});
	    }
	    break;
	case Shape::Value::wave: {
		BellStandingWaveDial shape(bPosition, bWidthInRing, (sqrt2 / 1.5f) *
		    secondsSinceBoot * waveWidth / 2.0f, waveWidth);
		renderList.push_back([bBlend, shape](float place){
		    return bBlend(shape(place));
		});
	    }
	    break;
	}
    }
    if (cWidthInRing) {
	switch (cShape.value) {
	case Shape::Value::bell: {
		BellDial shape(cPosition, cWidthInRing);
		renderList.push_back([cBlend, shape](float place){
		    return cBlend(shape(place));
		});
	    }
	    break;
	case Shape::Value::wave: {
		BellStandingWaveDial shape(cPosition, cWidthInRing,
		    secondsSinceBoot * waveWidth / 2.0f, waveWidth);
		renderList.push_back([cBlend, shape](float place){
		    return cBlend(shape(place));
		});
	    }
	    break;
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

    {
	SPI::Transaction transaction0(spiDevice[0], SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
	SPI::Transaction transaction1(spiDevice[1], SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
    }
}

static char const * const scoreKey[] = {"aScore", "bScore"};

void CornholeArtTask::scoreIncrement(size_t index, unsigned count) {
    io.post([this, index, count](){
	// increment the score but not above max
	unsigned value = std::min(maxScore, score[index] + 1 + count);
	std::ostringstream os;
	os << value;
	keyValueBroker.publish(scoreKey[index], os.str().c_str());
    });

}

void CornholeArtTask::scoreDecrement(size_t index, int count) {
    io.post([this, index, count](){
	if (0 > count) return;
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
    unsigned value = fromString<unsigned>(value_);
    io.post([this, index, value](){
	score[index] = value;
    });
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
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask},
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
	{keyValueBroker, "aScore", "0",
	    [this](char const * value){scoreObserved(0, value);}},
	{keyValueBroker, "bScore", "0",
	    [this](char const * value){scoreObserved(1, value);}},
    }
{
    pinTask.start();
}

/* virtual */ void CornholeArtTask::run() {
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

/* virtual */ CornholeArtTask::~CornholeArtTask() {
    stop();
    ESP_LOGI(name, "~CornholeArtTask");
}
