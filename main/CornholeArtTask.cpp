#include <esp_log.h>

#include "clip.h"
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

    std::list<std::function<LEDI(float)>> renderList;

    // clock
    {
	static float waveWidth = 2.0f / ringSize;
	float secondsSinceTwelveLocaltime
	    = smoothTime.millisecondsSinceTwelveLocaltime(microsecondsSinceBoot)
		/ millisecondsPerSecond;
	if (aWidth) {
	    float width = aWidth / ringSize;
	    float position = hourPulse(inDayOf(secondsSinceTwelveLocaltime));
	    switch (aShape.value) {
	    case Shape::Value::bell: {
		    BellDial shape(position, width);
		    renderList.push_back([aBlend, shape](float place){
			return aBlend(shape(place));
		    });
		}
		break;
	    case Shape::Value::wave: {
		    BellStandingWaveDial shape(position, width, (phi / 1.5f) *
			secondsSinceBoot * waveWidth / 2.0f, waveWidth);
		    renderList.push_back([aBlend, shape](float place){
			return aBlend(shape(place));
		    });
		}
		break;
	    }
	}
	if (bWidth) {
	    float width = bWidth / ringSize;
	    float position = minutePulse(inHourOf(secondsSinceTwelveLocaltime));
	    switch (bShape.value) {
	    case Shape::Value::bell: {
		    BellDial shape(position, width);
		    renderList.push_back([bBlend, shape](float place){
			return bBlend(shape(place));
		    });
		}
		break;
	    case Shape::Value::wave: {
		    BellStandingWaveDial shape(position, width, (sqrt2 / 1.5f) *
			secondsSinceBoot * waveWidth / 2.0f, waveWidth);
		    renderList.push_back([bBlend, shape](float place){
			return bBlend(shape(place));
		    });
		}
		break;
	    }
	}
	if (cWidth) {
	    float width = cWidth / ringSize;
	    float position = secondPulse(inMinuteOf(secondsSinceTwelveLocaltime));
	    switch (cShape.value) {
	    case Shape::Value::bell: {
		    BellDial shape(position, width);
		    renderList.push_back([cBlend, shape](float place){
			return cBlend(shape(place));
		    });
		}
		break;
	    case Shape::Value::wave: {
		    BellStandingWaveDial shape(position, width,
			secondsSinceBoot * waveWidth / 2.0f, waveWidth);
		    renderList.push_back([cBlend, shape](float place){
			return cBlend(shape(place));
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
	    for (auto & draw: renderList) {
		led = led + draw(place);
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
	    [this](unsigned count){ESP_LOGI(name, "button a press %d", count);},
	    [this](unsigned count){ESP_LOGI(name, "button a hold %d" , count);}
	},
	{pin[1], 0, bounceDuration, bufferDuration, holdDuration,
	    [this](unsigned count){ESP_LOGI(name, "button b press %d", count);},
	    [this](unsigned count){ESP_LOGI(name, "button b hold %d" , count);}
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
