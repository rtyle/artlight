#include <cmath>

#include <esp_log.h>

#include "Dial.h"
#include "InRing.h"
#include "Ring.h"
#include "Timer.h"

#include "function.h"

using APA102::LED;

namespace Ring {

using LEDI = APA102::LED<int>;

static float constexpr pi	= std::acos(-1.0f);
static float constexpr tau	= 2.0f * pi;
static float constexpr phi	= (1.0f + std::sqrt(5.0f)) / 2.0f;
static float constexpr sqrt2	= std::sqrt(2.0f);

static auto constexpr millisecondsPerSecond	= 1000u;

static size_t constexpr ringSize = 80;

// implicit unsigned integral conversions
// widen with zero extension and narrow by truncation
// this explicit narrowing conversion
// clips to max if larger and min if smaller.
template <typename T, typename U>
U clip(T t) {
    U max = std::numeric_limits<U>::max();
    if (t > static_cast<T>(max)) return max;
    U min = std::numeric_limits<U>::min();
    if (t < static_cast<T>(min)) return min;
    return t;
}

template <typename T, typename U = uint8_t>
LED<U> clip(LED<T> t) {
    U max = std::numeric_limits<U>::max();
    U min = std::numeric_limits<U>::min();
    LED<T> maxT(max, max, max);
    LED<T> minT(min, min, min);
    return LED<U>(t.minByPart(maxT).maxByPart(minT));
}

/// Pulse is a monotonically increasing function (object)
/// that, when compared to its Identity function,
/// pulses count times over a period of 1.
/// A count of 0 is the identity function.
/// The function can be made to pulseFirst at 0 or not.
class Pulse {
private:
    float const factor;
    float const sign;
public:
    Pulse(float count = 1, bool pulseFirst = false) :
	factor(count * tau),
	sign(pulseFirst ? 1 : -1)
    {}
    float operator () (float x) const {
	// factor in sin(x * factor) will adjust the frequency as needed.
	// dividing the result by the same factor will adjust the magnitude
	// such that the magnitude of the slope will never be less than -1.
	// when adding this to a curve with a slope of 1 everywhere
	// (the identity, x) this ensures the result monotonically increases.
	return factor ? x + sign * std::sin(x * factor) / factor : x;
    };
};

/// Sawtooth is a function object defined by its period.
/// Its output ranges from 0 to <1.
class Sawtooth {
private:
    float period;
public:
    Sawtooth(float period_ = 1.0f) : period(period_) {}
    float operator()(float x) {
	return std::modf(x / period, &x);
    }
};

static Sawtooth inSecondOf(1.0f);
static Sawtooth inMinuteOf(60.0f);
static Sawtooth inHourOf  (60.0f * 60.0f);
static Sawtooth inDayOf   (60.0f * 60.0f * 12.0f);	/// 12 hour clock

/// Blend is a function (object) that blends two values (of type T)
/// using linear interpolation
template <typename T> class Blend {
private:
    T const a;
    T const b;
    T const slope = b - a;
public:
    Blend(T a_, T b_) : a(a_), b(b_) {}
    template <typename F> T operator ()(F f) const {return slope * f + a;}
};

/// Art is an abstract base class for a function object that can return
/// an LEDI value for a specified place on a ring.
class Art {
public:
    virtual LEDI operator()(float place) const = 0;
};

static Pulse hourPulse	(0);//12);
static Pulse minutePulse(0);//60);
static Pulse secondPulse(0);//60);

/// Clock is Art that is constructed with the current time
/// and the width & color of each hour, minute and second hand.
template <typename Shape>
class Clock : public Art {
private:
    static float constexpr waveWidth = 2.0f / ringSize;
    float const		hTime;
    float const		mTime;
    float const		sTime;
    Blend<LEDI> const &	hBlend;
    Blend<LEDI> const &	mBlend;
    Blend<LEDI> const &	sBlend;
    Shape		hShape;
    Shape		mShape;
    Shape		sShape;
public:
    Clock(
	float			time,
	float			localTime,
	float			hWidth,
	Blend<LEDI> const &	hBlend_,
	float			mWidth,
	Blend<LEDI> const &	mBlend_,
	float			sWidth,
	Blend<LEDI> const &	sBlend_)
    :
	hTime	(hourPulse  (inDayOf   (localTime))),
	mTime	(minutePulse(inHourOf  (localTime))),
	sTime	(secondPulse(inMinuteOf(localTime))),
	hBlend	(hBlend_),
	mBlend	(mBlend_),
	sBlend	(sBlend_),
	hShape	(hTime, hWidth,
		    (phi / 1.5f) *	time * waveWidth / 2.0f, waveWidth),
	mShape	(mTime, mWidth,
		    (sqrt2 / 1.5f) *	time * waveWidth / 2.0f, waveWidth),
	sShape	(sTime, sWidth,
					time * waveWidth / 2.0f, waveWidth)
    {}
    /*virtual */ LEDI operator()(float place) const {
	return	  hBlend(hShape(place))
		+ mBlend(mShape(place))
		+ sBlend(sShape(place))
	;
    }
};

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

void ArtTask::update() {
    float time = esp_timer_get_time() / 1000000.0f;

    static BumpDial dial;

    Blend<LEDI> aBlend {LEDI(aFades), LEDI(aColor)};
    Blend<LEDI> bBlend {LEDI(bFades), LEDI(bColor)};
    Blend<LEDI> cBlend {LEDI(cFades), LEDI(cColor)};

    float place = time / 9.0f;

    LEDC::Channel (*rgb)[3] = &ledChannel[0];
    updateLedChannelRGB(*rgb++, aBlend(dial(place * phi  )));
    updateLedChannelRGB(*rgb++, bBlend(dial(place * sqrt2)));
    updateLedChannelRGB(*rgb++, cBlend(dial(place * 1.5f )));

    Clock<BellStandingWaveDial> art(
	time,
	static_cast<float>(smoothTime.millisecondsSinceTwelveLocaltime())
	    / millisecondsPerSecond,
	aWidth / ringSize, aBlend,
	bWidth / ringSize, bBlend,
	cWidth / ringSize, cBlend);

    #if 0
	FoldsInRing inRing(12, 11);
    #else
	OrdinalsInRing inRing(ringSize);
    #endif

    LEDI leds[ringSize];

    // render art from places in the ring,
    // keeping track of the largest led value by part.
    auto maxRendering = std::numeric_limits<int>::min();
    for (auto & led: leds) {
	for (auto & place: *inRing) {
	    led = led + art(place);
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
	SPI::Transaction transaction1(spiDevice1, SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
	SPI::Transaction transaction2(spiDevice2, SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
    }
}

static unsigned constexpr bounceDuration	=  25000;
static unsigned constexpr bufferDuration	= 150000;
static unsigned constexpr holdDuration		= 500000;

ArtTask::ArtTask(
    SPI::Bus const *		spiBus1,
    SPI::Bus const *		spiBus2,
    ObservablePin		(&pin_)[4],
    LEDC::Channel		(&ledChannel_)[3][3],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ::ArtTask		("ringArtTask", 5, 16384, 1,
			spiBus1, spiBus2, pin_, ledChannel_,
			getLux_, keyValueBroker_),

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
    }
{}

/* virtual */ void ArtTask::run() {
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

/* virtual */ ArtTask::~ArtTask() {
    stop();
    ESP_LOGI(name, "~Ring::ArtTask");
}

}
