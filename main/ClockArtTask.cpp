#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

#include <stdlib.h>
extern "C" int setenv(char const *, char const *, int);

#include <esp_log.h>

#include "APA102.h"
#include "ClockArtTask.h"
#include "Timer.h"

#include "fromString.h"

using APA102::LED;

static auto constexpr millisecondsPerSecond	= 1000u;
static auto constexpr millisecondsPerMinute	= 60u * millisecondsPerSecond;
static auto constexpr millisecondsPerHour	= 60u * millisecondsPerMinute;

static auto constexpr microsecondsPerMillisecond = 1000u;

#if __cplusplus >= 201402L
template <typename T, std::size_t N>
T constexpr sum(T const (&addends)[N]) {
    T result = 0; for (auto addend : addends) result += addend; return result;
}
#endif


/// Pulse is a monotonically increasing function (object)
/// for a floating type F that, when compared to its Identity function,
/// pulses count times over a period of 1.
/// A count of 0 is the identity function.
/// The function can be made to pulseFirst at 0 or not.
template <typename F = float> class Pulse {
private:
    F const factor;
    F const sign;
public:
    Pulse(F count = 1, bool pulseFirst = false) :
	factor(count * 2 * std::acos(static_cast<F>(-1.0)) /* pi */),
	sign(pulseFirst ? 1 : -1)
    {}
    F operator () (F x) const {
	// factor in sin(x * factor) will adjust the frequency as needed.
	// dividing the result by the same factor will adjust the magnitude
	// such that the magnitude of the slope will never be less than -1.
	// when adding this to a curve with a slope of 1 everywhere
	// (the identity, x) this ensures the result monotonically increases.
	return factor ? x + sign * std::sin(x * factor) / factor : x;
    };
};

/// Bell is a standard normal distribution (bell) curve ...
/// http://wikipedia.org/wiki/Normal_distribution
/// ... function (object)
/// parameterized by its standard deviation (sigma) and mean.
/// By default, the mean value is calculated
/// so that the total area under the curve (probability) is 1.
/// This can be overridden to get a curve with the same shape
/// but a different mean value.
template <typename F = float> class Bell : public std::function<F(F)> {
private:
    F const twoSigmaSquared;
    F const mean;
    F const meanValue;
public:
    Bell(F sigma = 1, F mean_ = 0) :
	std::function<F(F)>([this](F x){return (*this)(x);}),
	twoSigmaSquared(2 * sigma * sigma),
	mean(mean_),
	meanValue(static_cast<F>(1.0)
	    / std::sqrt(std::acos(static_cast<F>(-1.0)) /* pi */
	    * twoSigmaSquared))
    {}
    Bell(F sigma, F mean_, F meanValue_) :
	twoSigmaSquared(2 * sigma * sigma),
	mean(mean_),
	meanValue(meanValue_)
    {}
    F operator () (F x) const {
	x -= mean;
	return meanValue * std::exp(-x * x / twoSigmaSquared);
    }
};

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

/// RowIndex provides a way to index packed ragged rows
/// (rows that may not all have the same number of columns).
template <size_t N> class RowIndex {
public:
    std::array<size_t, N> const	rowSizes;
    size_t const		packedSize;
    size_t const		maxRowSize;
    std::array<size_t, N> const	rowBegins;
    RowIndex(std::array<size_t, N> const rowSizes_) :
	rowSizes(rowSizes_),
	packedSize(std::accumulate(rowSizes.begin(), rowSizes.end(), 0)),
	maxRowSize(*std::max_element(rowSizes.begin(), rowSizes.end())),
	rowBegins([this](){
	    std::array<size_t, N> result;
	    auto rowSize = rowSizes.begin();
	    auto rowBegin = result.begin();
	    size_t previousPackedSize = 0;
	    while (rowSize != rowSizes.end()) {
		*rowBegin++ = previousPackedSize;
		previousPackedSize += *rowSize++;
	    }
	    return result;
	}())
    {}
    #if __cplusplus >= 201402L
    RowIndex(size_t const (&rowSizes)[N])
	: RowIndex(std::experimental::to_array(rowSizes)) {};
    #endif
    size_t size() const {return packedSize;}
    size_t size(size_t row) const {return rowSizes[row];}
    size_t operator [] (size_t row) const {return rowBegins[row];}
};

/// Fraction (expressed as its numerator and denominator of type I)
/// is a function (object) that returns an equivalent floating point number.
template <typename I = unsigned> class Fraction {
public:
    I const numerator;
    I const denominator;
    Fraction(I numerator_ = 0, I denominator_ = 1) :
	numerator(numerator_), denominator(denominator_) {}
    template <typename F> F operator * (F f) const
	{return numerator * f / denominator;}
    template <typename F> F operator * (Fraction const & that) const {
	return static_cast<F>(numerator) * that.numerator
	    / denominator / that.denominator;
    }
    template <typename F> operator F() const {
	return static_cast<F>(numerator) / denominator;
    }
};

/// Mixed (number, expressed as its whole and proper Fraction parts of type I)
/// is a function (object) that returns its whole part.
template <typename I = unsigned> class Mixed {
public:
    I const whole;
    Fraction<I> const part;
    Mixed() : whole(0) {}
    Mixed(Fraction<I> f) :
	whole(f.numerator / f.denominator),
	part(f.numerator % f.denominator, f.denominator)
    {}
    operator I() const {return whole;}
};

/// Wrap's increment/decrement operators will wrap a pointer
/// around a circular array
template <typename T> class Wrap {
private:
    T * const begin;
    T * const end;
    T * index;
public:
    Wrap(T * begin_, size_t count, size_t index_) :
	begin(begin_), end(begin + count), index(begin + index_ % count) {}
    Wrap & operator ++() {
	++index; if (index == end) {index = begin;} return *this;
    }
    Wrap & operator --() {
	if (index == begin) {index = end;} --index; return *this;
    }
    operator T * () const {return index;}
};

class Rendering {
private:
    static unsigned constexpr HPD = 12;	// 12 hour clock
    static unsigned constexpr MPH = 60;
    static unsigned constexpr SPM = 60;

    // for hours,
    // full circle is fragmented into folded rays with connecting arcs
    static size_t constexpr rayCount		= HPD;
    static size_t constexpr rayFoldLength	= 3;
    static size_t constexpr rayArcLength	= 2;
    static size_t constexpr rayLength
	= 1 + rayFoldLength + rayArcLength + rayFoldLength;

    // for minutes and seconds,
    // full circle is fragmented into arcs of unequal length
    static size_t constexpr arcCount = 12;
    static std::array<size_t, arcCount> const arcLengths;
    static size_t constexpr circleLength = 72;
    static RowIndex<arcCount> const arcIndex;

    // subordinate increment (h, m, s) per superior increment (d, h, m)
    static unsigned constexpr hpd = rayCount * rayLength;
    static unsigned constexpr mph = circleLength;
    static unsigned constexpr spm = circleLength;

    static Pulse<> const hp, mp, sp;

public:
    APA102::Message<hpd - rayCount * rayFoldLength + circleLength> message;

    Rendering(
	unsigned			now,
	unsigned			tps,
	float				dim,
	LED<>				glow,
	std::function<LED<int>(float)>	hf,
	std::function<LED<int>(float)>	mf,
	std::function<LED<int>(float)>	sf)
    {
	// ticks per clock unit (s, m, h, d)
	unsigned tpm(tps * SPM);
	unsigned tph(tpm * MPH);
	unsigned tpd(tph * HPD);

	// Indices corresponding to time now
	Mixed<> D(Fraction<>(now, tpd));
	Mixed<> H(Fraction<>(D.part.numerator, tph));
	Mixed<> M(Fraction<>(H.part.numerator, tpm));
	Mixed<> S(Fraction<>(M.part.numerator, tps));

	static uint32_t const off = LED<>(0, 0, 0);

	uint32_t * target;

	{
	    // render hours unfolded
	    uint32_t unfolded[hpd];
	    target = unfolded;
	    for (auto i = target + hpd; i > target;) *--i = off;
	    float c;
	    float ud = - std::modf(hp(D.part) * hpd, &c);
	    if (-0.5f > ud) {
		c += 1;
		ud += 1;
	    }
	    float dd = ud;
	    Wrap<uint32_t> up(target, hpd, c), dp(up);
	    *up = hf(ud) * dim;
	    for (size_t z = 8; z--;) {
		*++up = hf(ud += 1) * dim;
		*--dp = hf(dd -= 1) * dim;
	    }
	    // fold/collapse hour rendering, mixing folded values
	    uint32_t const * source = target;
	    uint32_t const * fold = target + hpd - 1;
	    uint32_t const * next = target + rayLength - 1;
	    for (size_t i = rayCount; i--;) {
		*target++ = *source++;
		for (size_t i = rayFoldLength; i--; ++source, --fold) {
		    *target++ = LED<>(*source).maxByPart(LED<>(*fold));
		}
		fold = next;
		next += rayLength;
		for (size_t i = rayArcLength; i--;)
		    *target++ = *source++;
		source += rayFoldLength;
	    }
	    // mix in background glow
	    LED<> background = glow * dim;
	    for (auto & e: unfolded) {
		e = LED<>(e).maxByPart(background);
	    }
	    // transfer folded part of rendering into message
	    size_t foldedSize = target - unfolded;
	    std::memcpy(message.encodings, unfolded, foldedSize * sizeof *target);
	    target = message.encodings + foldedSize;
	}

	// clear shared rendering for minutes and seconds
	for (auto i = target + circleLength; i > target;) *--i = off;

	size_t const maxCircleLength = arcCount * arcIndex.maxRowSize;

	// render minutes
	{
	    float c = mp(H.part) * maxCircleLength;
	    size_t arc = c / arcIndex.maxRowSize;
	    c -= arc * arcIndex.maxRowSize;
	    float ud = - std::modf(c * arcIndex.size(arc) / arcIndex.maxRowSize, &c);
	    if (-0.5f > ud) {
		c += 1;
		ud += 1;
	    }
	    float dd = ud;
	    c += arcIndex[arc];
	    Wrap<uint32_t> up(target, mph, c), dp(up);
	    *up = mf(ud) * dim;
	    for (size_t z = 8; z--;) {
		*++up = mf(ud += 1) * dim;
		*--dp = mf(dd -= 1) * dim;
	    }
	}

	// mix seconds over minutes
	{
	    float c = mp(M.part) * maxCircleLength;
	    size_t arc = c / arcIndex.maxRowSize;
	    c -= arc * arcIndex.maxRowSize;
	    float ud = - std::modf(c * arcIndex.size(arc) / arcIndex.maxRowSize, &c);
	    if (-0.5f > ud) {
		c += 1;
		ud += 1;
	    }
	    float dd = ud;
	    c += arcIndex[arc];
	    Wrap<uint32_t> up(target, mph, c), dp(up);
	    *up = LED<int>(*up).maxByPart(LED<int>(sf(ud)) * dim);
	    for (size_t z = 4; z--;) {
		++up; *up = LED<int>(*up).maxByPart(LED<int>(sf(ud += 1)) * dim);
		--dp; *dp = LED<int>(*dp).maxByPart(LED<int>(sf(dd -= 1)) * dim);
	    }
	}

	target += circleLength;

	assert(target - message.encodings == sizeof message.encodings / sizeof *message.encodings);
    }
};

/* static */ std::array<size_t, Rendering::arcCount> const
    Rendering::arcLengths =
	// {4, 8, 4, 8, 4, 8, 4, 8, 4, 8, 4, 8};
	   {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};
/* static */ RowIndex<Rendering::arcCount> const
    Rendering::arcIndex(arcLengths);

/* static */ Pulse<> const Rendering::hp(HPD);
/* static */ Pulse<> const Rendering::mp(MPH);
/* static */ Pulse<> const Rendering::sp(SPM);

// we want to get (and present) local time of day at a subsecond resolution.
// the resolution of std::time is a second.
// the resolution of clock_gettime(CLOCK_REALTIME,...) is a nanosecond.
// these functions are implemented in time.c of the newlib component of esp-idf.
// the resolution of time keeping on the esp32 platform is a microsecond.
//
// 1. because their is no battery backed timekeeper, time is lost when power is.
// 2. time will drift due to the inaccuracy of the platform's hardware timer.
// we expect both of these issues to be addressed with lwIP SNTP.
// lwIP SNTP does not use adjtime to slew small time drifts (2).
// rather, it always uses settimeofday to resolve big (1) and small time
// differences.
//
// the newlib implementation of settimeofday (and adjtime) just jumps (or slews)
// a private "boot time" variable in time.c.
// this variable is set so that when the platform's
// "microseconds since boot time" measure is added to it
// we get the current time.
//
// we would like our presentation of time to not immediately jump
// to the SNTP corrected time.
// rather, we would like to see it gradually snap into place.
// this way, hour, minute and second hands might animate as they would on an
// analog clock when someone spins the second hand to change the time.
//
// to do this, we need to know when these jumps occur and how big they are.
// different timers might be compared to do this but it would sacrifice
// some accuracy (time elapses between samples) and risk being scheduled out
// between samples.
// a simpler, more reliable implementation would be to rely on being able
// to access the private "boot time" variable in time.c
// so, that is what we have done (see our modified copy here).
//
// although time.c advertises this as uint64_t, at boot time it has been seen
// to wrap backwards from 0 about a second (a negative number).
// so, we will interpret it as such.
extern "C" int64_t get_boot_time();

ClockArtTask::SmoothTime::SmoothTime(char const * name_, size_t stepCount_)
:
    name(name_),
    stepCount(stepCount_),
    stepLeft(0),
    stepProduct(0),
    stepFactor(0),
    lastBootTime(get_boot_time())
{}

int64_t ClockArtTask::SmoothTime::microsecondsSinceEpoch() {
    uint64_t microseconds = esp_timer_get_time();
    int64_t const thisBootTime = get_boot_time();
    if (thisBootTime != lastBootTime) {
	// if we were to cut the difference in half on each successive step
	// there would be log2(abs(thisBootTime - lastBootTime)) steps needed.
	// the corrected boot time on step N would be
	//	thisBootTime - (thisBootTime - lastBootTime) * 1 / 2**N
	// this can be optimized if we accumulate a stepProduct on each step
	// so that the corrected boot time on the next step would be
	//	thisBootTime - (stepProduct = stepProduct * stepFactor)
	// stepProduct would start as thisBootTime - lastBootTime.
	// to cut the difference in half each time, stepFactor would be 1/2.
	// to stretch the number of steps to make the same transition,
	// we just need to affect the stepFactor (see below).
	stepProduct  = thisBootTime - lastBootTime;
	lastBootTime = thisBootTime;

	// since our result is used to animate a 12 hour clock
	// it makes no sense to step over a larger amount.
	// remove full turns to get within one turn centered on thisBootTime.
	static auto constexpr turn = 12 * 60 * 60 * 1000000ull;
	static auto constexpr halfTurn = turn / 2;
	if (0.0f < stepProduct) {
	    stepProduct -= (
		(static_cast<uint64_t>( stepProduct) + halfTurn) / turn) * turn;
	} else {
	    stepProduct += (
		(static_cast<uint64_t>(-stepProduct) + halfTurn) / turn) * turn;
	}

	if (stepProduct) {
	    stepLeft = stepCount;
	    float halfStepCount
		= std::log2(0.0f < stepProduct ? stepProduct : -stepProduct);
	    stepFactor = std::exp2(-1.0f * halfStepCount / stepCount);
//	    ESP_LOGI(name, "thisBootTime=%lld stepProduct=%f halfStepCount=%f stepCount=%zu stepFactor=%f",
//		thisBootTime, stepProduct, halfStepCount, stepCount, stepFactor);
	}
    }
    if (!stepLeft) {
//	ESP_LOGI(name, "%llu (now) = %lld (thisBootTime) + %llu (us)",
//	    thisBootTime + microseconds, thisBootTime, microseconds);
	return thisBootTime + microseconds;
    } else {
	--stepLeft;
	stepProduct *= stepFactor;
	int64_t now = thisBootTime - static_cast<int64_t>(stepProduct) + microseconds;
//	ESP_LOGI(name, "%lld (now) = %lld (thisBootTime) - %lld (stepProduct) + %llu (us)",
//	    now, thisBootTime, static_cast<int64_t>(stepProduct), microseconds);
	return now;
    }
}

uint32_t ClockArtTask::SmoothTime::millisecondsSinceTwelveLocaltime() {
    int64_t milliseconds
	= microsecondsSinceEpoch() / microsecondsPerMillisecond;

    std::time_t seconds = milliseconds / millisecondsPerSecond;
    milliseconds -= static_cast<int64_t>(seconds) * millisecondsPerSecond;

    std::tm tm;
    localtime_r(&seconds, &tm);

    return milliseconds
	+ (tm.tm_sec      ) * millisecondsPerSecond
	+ (tm.tm_min      ) * millisecondsPerMinute
	+ (tm.tm_hour % 12) * millisecondsPerHour;
}

template <typename A, typename B, typename C>
std::function<C(A)> compose(
	std::function<C(B)> f, std::function<B(A)> g) {
    return [f, g](A a) {return f(g(a));};
}

void ClockArtTask::update() {
    // dim factor is calculated as a function of the current ambient lux.
    // this will range from 1/16 to 16/16 with the numerator increasing by
    // 1 as the lux doubles.
    float dim = (1.0f + std::min(15.0f, std::log2(1.0f + getLux()))) / 16.0f;
    Rendering rendering(
	smoothTime.millisecondsSinceTwelveLocaltime(),
	millisecondsPerSecond,
	dim,
	hourGlow,
	compose(
	    std::function<LED<int>(float)>(
		Ramp<LED<int>>(LED<int>(hourTail), LED<int>(hourMean))),
	    Bell<float>(hourShape, 0.0f, 1.0f)
	),
	compose(
	    std::function<LED<int>(float)>(
		Ramp<LED<int>>(LED<int>(minuteTail), LED<int>(minuteMean))),
	    Bell<float>(minuteShape, 0.0f, 1.0f)
	),
	compose(
	    std::function<LED<int>(float)>(
		Ramp<LED<int>>(LED<int>(secondTail), LED<int>(secondMean))),
	    Bell<float>(secondShape, 0.0f, 1.0f)
	));
    {
	SPI::Transaction transaction1(spiDevice1, SPI::Transaction::Config()
	    .tx_buffer_(&rendering.message)
	    .length_(rendering.message.length()));
	SPI::Transaction transaction2(spiDevice2, SPI::Transaction::Config()
	    .tx_buffer_(&rendering.message)
	    .length_(rendering.message.length()));
    }
}

ClockArtTask::ClockArtTask(
    SPI::Bus const *		spiBus1,
    SPI::Bus const *		spiBus2,
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    AsioTask		("clockArtTask", 5, 16384, 1),

    spiDevice1		(spiBus1, SPI::Device::Config()
			    .mode_(APA102::spiMode)
			    .clock_speed_hz_(16000000)	// see SPI_MASTER_FREQ_*
			    .spics_io_num_(-1)		// no chip select
			    .queue_size_(1)
			),
    spiDevice2		(spiBus2, SPI::Device::Config()
			    .mode_(APA102::spiMode)
			    .clock_speed_hz_(16000000)	// see SPI_MASTER_FREQ_*
			    .spics_io_num_(-1)		// no chip select
			    .queue_size_(1)
			),

    getLux		(getLux_),

    keyValueBroker	(keyValueBroker_),

    // timezone affects our notion of the localtime we present
    // forward a copy of any update to our task to make a synchronous change
    timezoneObserver(keyValueBroker, "timezone", CONFIG_TIME_ZONE,
	[this](char const * timezone){
	    std::string timezoneCopy(timezone);
	    io.post([this, timezoneCopy](){
		ESP_LOGI(name, "timezone %s", timezoneCopy.c_str());
		setenv("TZ", timezoneCopy.c_str(), true);
	    });
	}),

    hourShape		(1.0f),
    hourMean		(0u),
    hourTail		(0u),
    hourGlow		(0u),
    hourShapeObserver(keyValueBroker, "hourShape", "2.7",
	[this](char const * shapeObserved){
	    float shape = fromString<float>(shapeObserved);
	    io.post([this, shape](){
		hourShape = shape;
	    });
	}),
    hourMeanObserver(keyValueBroker, "hourMean", "#ff0000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		hourMean = led;
	    });
	}),
    hourTailObserver(keyValueBroker, "hourTail", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		hourTail = led;
	    });
	}),
    hourGlowObserver(keyValueBroker, "hourGlow", "#000000",
	[this](char const * color){
	    LED<unsigned> led(color);
	    static unsigned constexpr brightest = 64;
	    unsigned brightness = led.sum();
	    if (brightest < brightness) {
		std::string dimmed(LED<>(led * brightest / brightness));
		keyValueBroker.publish("hourGlow", dimmed.c_str());
	    } else {
		io.post([this, led](){
		    hourGlow = led;
		});
	    }
	}),

    minuteShape		(1.0f),
    minuteMean		(0u),
    minuteTail		(0u),
    minuteShapeObserver(keyValueBroker, "minuteShape", "2.7",
	[this](char const * shapeObserved){
	    float shape = fromString<float>(shapeObserved);
	    io.post([this, shape](){
		minuteShape = shape;
	    });
	}),
    minuteMeanObserver(keyValueBroker, "minuteMean", "#0000ff",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		minuteMean = led;
	    });
	}),
    minuteTailObserver(keyValueBroker, "minuteTail", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		minuteTail = led;
	    });
	}),

    secondShape		(1.0f),
    secondMean		(0u),
    secondTail		(0u),
    secondShapeObserver(keyValueBroker, "secondShape", "1.4",
	[this](char const * shapeObserved){
	    float shape = fromString<float>(shapeObserved);
	    io.post([this, shape](){
		secondShape = shape;
	    });
	}),
    secondMeanObserver(keyValueBroker, "secondMean", "#ffff00",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		secondMean = led;
	    });
	}),
    secondTailObserver(keyValueBroker, "secondTail", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		secondTail = led;
	    });
	}),

    smoothTime	("smoothTime", 4096)
{}

/* virtual */ void ClockArtTask::run() {
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

/* virtual */ ClockArtTask::~ClockArtTask() {
    stop();
    ESP_LOGI(name, "~ClockArtTask");
}
