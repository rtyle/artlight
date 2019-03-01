#include <cmath>

#include <esp_log.h>

#include "InRing.h"
#include "Ring.h"
#include "Timer.h"

#include "function.h"

using APA102::LED;

namespace Ring {

using LEDI = APA102::LED<int>;

static auto const pi = std::acos(-1.0f);

static auto constexpr millisecondsPerSecond	= 1000u;

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
	factor(count * 2 * pi),
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

/// Ramp is a function (object) that outputs a linear ramp between two output
/// values of type T as its input goes from 0 to 1.
template <typename T> class Ramp {
private:
    T const a;
    T const b;
    T const slope = b - a;
public:
    Ramp(T a_, T b_) : a(a_), b(b_) {}
    template <typename F> T operator ()(F f) const {return slope * f + a;}
};

/// Moving describes how something moves with respect to time
struct Moving {
public:
    float const center;		/// place at time 0
    float const speed;		/// change in place per unit time
    Moving(float center_ = 0.0f, float speed_ = 0.0f)
	: center(center_), speed(speed_) {}
};

/// At describes where something is at (place and time).
/// With a Moving object, it can tell how far from the Moving.center it is.
/// Moving objects move around a circle with a perimeter of 1.
/// The calculated distance to the Moving.center will be between [-0.5, 0.5).
struct At {
public:
    float const	place;
    float const	time;
    At(float place_, float time_) : place(place_), time(time_) {}
    float center(Moving const & moving) const {
	float distance = place - moving.center - time * moving.speed;
	float ignore;
	if (distance < 0.0f) {
	    distance = 1.0f - std::modf(-distance, &ignore);
	} else {
	    distance = std::modf(distance, &ignore);
	}
	if (distance >= 0.5f) distance -= 1.0f;
	return distance;
    }
};

/// Bump is a Moving (from a center with a speed)
/// rectified cosine function object
/// with a frequency (number of periods in a domain span of 1).
/// The output is adjusted to range from 0 to 1.
class Bump : public Moving {
private:
    float const	frequency;
public:
    Bump(
	float center		= 0.0f,
	float speed		= 0.0f,
	float width		= 1.0f)
    :
	Moving		(center, speed),
	frequency	(1.0f / (2.0f * width))
    {}
    float operator()(At at) const {
	return std::abs(std::cos(2.0f * pi * frequency * at.center(*this)));
    }
};

/// Bell is a Moving (from a center with a speed)
/// standard normal distribution curve function object
/// with a standard deviation and center value of 1.
/// http://wikipedia.org/wiki/Normal_distribution
class Bell : public Moving {
private:
    float const twoSigmaSquared;
public:
    Bell(
	float center	= 0.0f,
	float speed	= 0.0f,
	float sigma	= 1.0f)
    :
	Moving		(center, speed),
	twoSigmaSquared	(2 * sigma * sigma)
    {}
    float operator()(At at) const {
	float place = at.center(*this);
	return std::exp(-place * place / twoSigmaSquared);
    }
};

/// Wave is a Moving (from a center with a speed)
/// cosine function object
/// with a frequency (number of periods in a domain span of 1).
/// The output is adjusted to range from 0 to 1.
class Wave : public Moving {
private:
    float const	frequency;
public:
    Wave(
	float center		= 0.0f,
	float speed		= 0.0f,
	float width		= 1.0f)
    :
	Moving		(center, speed),
	frequency	(1.0f / width)
    {}
    float operator()(At at) const {
	return (1.0f + std::cos(2.0f * pi * frequency * at.center(*this)))
	    / 2.0f;
    }
};

/// BellWave composes a Bell after a right and left Moving Wave.
class BellWave : public Moving {
private:
    static auto constexpr	divisor = 4.0f;
    Bell const		bell;
    Wave const		right;
    Wave const		left;
public:
    BellWave(
	float center		= 0.0f,
	float speed		= 0.0f,
	float width		= 1.0f)
    :
	bell	(center, speed, width),
	right	(center, 3.0f * speed, width / divisor),
//	left	(center, -speed * 3.0f / 2.0f, width / 4.0)
	left	(center, 0.0, width / divisor)
    {}
    float operator()(At at) const {
	return bell(at) * (right(at) + left(at)) / 2.0f;
    }
};

/// https://en.wikipedia.org/wiki/Sinc_function
template <typename F>
static F sinc(F x) {
    if (0.0f == x) return 1.0f;
    return std::sin(x) / x;
}

/// Ripple is a Moving (from a center with a speed)
/// modified sinc(x) (sin(x)/x) curve
/// whose output ranges from 0 to 1
/// https://www.desmos.com/calculator/9ndsrd9psf
class Ripple : public Moving {
private:
    float const width;
public:
    Ripple(
	float center	= 0.0f,
	float speed	= 0.0f,
	float width_	= 1.0f)
    :
	Moving		(center, speed),
	width		(width_)
    {}
    float operator()(At at) const {
	float place = pi * std::abs(at.center(*this) / width);
	if (1.0f / 2.0f > place) {
	    return sinc(place);
	} else {
	    return (1.0f + std::sin(place)) / (2.0f * place);
	}
    }
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
/// and the width & color (of the mean and tail(s))
/// of each hour, minute and second hand.
template <typename Shape>
class Clock : public Art {
private:
    float const		hTime;
    float const		mTime;
    float const		sTime;
    Ramp<LEDI> const &	hRamp;
    Ramp<LEDI> const &	mRamp;
    Ramp<LEDI> const &	sRamp;
    Shape		hShape;
    Shape		mShape;
    Shape		sShape;
public:
    Clock(
	float			time,	// seconds since 12, local time
	float			hWidth,
	Ramp<LEDI> const &	hRamp_,
	float			mWidth,
	Ramp<LEDI> const &	mRamp_,
	float			sWidth,
	Ramp<LEDI> const &	sRamp_)
    :
	hTime	(hourPulse  (inDayOf   (time))),
	mTime	(minutePulse(inHourOf  (time))),
	sTime	(secondPulse(inMinuteOf(time))),
	hRamp	(hRamp_),
	mRamp	(mRamp_),
	sRamp	(sRamp_),
	hShape	(0.0f, 1.0f, hWidth),
	mShape	(0.0f, 1.0f, mWidth),
	sShape	(0.0f, 1.0f, sWidth)
    {}
    /*virtual */ LEDI operator()(float place) const {
	return	//  hRamp(hShape(At(place, hTime)))
		//+ mRamp(mShape(At(place, mTime)))
		/*+*/ sRamp(sShape(At(place, sTime)))
	;
    }
};

void updateLedChannel(LEDC::Channel & ledChannel, uint32_t duty) {
    // do not use ledChannel.set_duty_and_update
    ledChannel.set_duty(duty);
    ledChannel.update_duty();
}

void updateLedChannels(LEDC::Channel (&ledChannels)[3], LEDI const & led) {
    LEDC::Channel * ledChannel = ledChannels;
    for (auto duty: {led.part.red, led.part.green, led.part.blue}) {
	updateLedChannel(*ledChannel++, duty);
    }
}

void ArtTask::update() {
    static float constexpr phi		= (1.0f + std::sqrt(5.0f)) / 2.0f;
    static float constexpr sqrt2	= std::sqrt(2.0f);

    Bump aShape(0.0f, phi   / 9.0f, 1.0f);	// < 6 seconds irrational
    Bump bShape(0.0f, sqrt2 / 9.0f, 1.0f);	// > 6 seconds irrational
    Bump cShape(0.0f, 1.5f  / 9.0f, 1.0f);	// = 6 seconds   rational

    Ramp<LEDI> aRamp {LEDI(aTail), LEDI(aMean)};
    Ramp<LEDI> bRamp {LEDI(bTail), LEDI(bMean)};
    Ramp<LEDI> cRamp {LEDI(cTail), LEDI(cMean)};

    float secondsSinceBoot = esp_timer_get_time() / 1000000.0f;

    LEDC::Channel (*rgb)[3] = &ledChannel[0];
    updateLedChannels(*rgb++, aRamp(aShape(At(0.0f, secondsSinceBoot))));
    updateLedChannels(*rgb++, bRamp(bShape(At(0.0f, secondsSinceBoot))));
    updateLedChannels(*rgb++, cRamp(cShape(At(0.0f, secondsSinceBoot))));

    static size_t constexpr perimeterLength = 80;

    Clock<BellWave> art(
	static_cast<float>(smoothTime.millisecondsSinceTwelveLocaltime())
	    / millisecondsPerSecond,
	aWidth / perimeterLength, aRamp,
	bWidth / perimeterLength, bRamp,
	cWidth / perimeterLength, cRamp);

    #if 0
	FoldsInRing inRing(12, 11);
    #else
	OrdinalsInRing inRing(perimeterLength);
    #endif

    LEDI leds[perimeterLength];

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

    APA102::Message<perimeterLength> message;

    /// adjust brightness
    float dimming = dim == Dim::manual
	? dimLevel / 16.0f
	// automatic dimming as a function of measured ambient lux.
	// this will range from 3/16 to 16/16 with the numerator increasing by
	// 1 as the lux doubles up until 2^13 (~full daylight, indirect sun).
	// an LED value of 128 will be dimmed to 24 in complete darkness (lux 0)
	: (3.0f + std::min(13.0f, std::log2(1.0f + getLux()))) / 16.0f;
    LEDI * led = leds;
    if (range == Range::clip) {
	for (auto & e: message.encodings) {
	    e = LED<>(gammaEncode,
		clip(*led++) * dimming);
	}
    } else if (range == Range::normalize) {
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

    observer {
	{pin[0], [this](){ESP_LOGI(name, "pin a %d" , pin[0].get_level());}},
	{pin[1], [this](){ESP_LOGI(name, "pin b %d" , pin[1].get_level());}},
	{pin[2], [this](){ESP_LOGI(name, "pin c %d" , pin[2].get_level());}},
	{pin[3], [this](){ESP_LOGI(name, "pin ir %d", pin[3].get_level());}},
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
