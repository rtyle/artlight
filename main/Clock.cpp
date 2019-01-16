#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

#include <esp_log.h>

#include "APA102.h"
#include "Clock.h"
#include "Timer.h"

#include "fromString.h"
#include "function.h"

using APA102::LED;

namespace Clock {

static auto constexpr millisecondsPerSecond	= 1000u;

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
/// This can be overridden to get a curve with the same width
/// but a different mean value.
template <typename F = float> class Bell {
private:
    F const twoSigmaSquared;
    F const mean;
    F const meanValue;
public:
    Bell(F sigma = 1, F mean_ = 0) :
	twoSigmaSquared(2 * sigma * sigma),
	mean(mean_),
	meanValue(static_cast<F>(1.0)
	    / std::sqrt(std::acos(-1.0f) /* pi */
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
	size_t				hw,
	size_t				mw,
	size_t				sw,
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
	    for (size_t z = hw; z--;) {
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
	    // add background glow where dimmest
	    LED<> background = glow * dim;
	    static uint8_t constexpr dimmest = 24;
	    for (auto & e: unfolded) {
		if (LED<>(e).max() < dimmest) e = background;
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
	    if (-0.5f >= ud) {
		++c;
		ud += 1;
	    }
	    float dd = ud;
	    c += arcIndex[arc];
	    Wrap<uint32_t> up(target, mph, c), dp(up);
	    *up = mf(ud) * dim;
	    for (size_t z = mw; z--;) {
		*++up = mf(ud += 1) * dim;
		*--dp = mf(dd -= 1) * dim;
	    }
	}

	// mix seconds over minutes
	{
	    float c = sp(M.part) * maxCircleLength;
	    size_t arc = c / arcIndex.maxRowSize;
	    c -= arc * arcIndex.maxRowSize;
	    float ud = - std::modf(c * arcIndex.size(arc) / arcIndex.maxRowSize, &c);
	    if (-0.5f >= ud) {
		++c;
		ud += 1;
	    }
	    float dd = ud;
	    c += arcIndex[arc];
	    Wrap<uint32_t> up(target, spm, c), dp(up);
	    *up = LED<int>(*up).maxByPart(LED<int>(sf(ud)) * dim);
	    for (size_t z = sw; z--;) {
		++up; *up = LED<int>(*up).maxByPart(LED<int>(sf(ud += 1)) * dim);
		--dp; *dp = LED<int>(*dp).maxByPart(LED<int>(sf(dd -= 1)) * dim);
	    }
	}

	target += circleLength;

	assert(target - message.encodings == sizeof message.encodings / sizeof *message.encodings);

	// apply APA102 gamma correction to rendered message
	message.gamma();
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

void ArtTask::update() {
    // dim factor is calculated as a function of the current ambient lux.
    // this will range from 3/16 to 16/16 with the numerator increasing by
    // 1 as the lux doubles up until 2^13 (~full daylight, indirect sun).
    // an LED value of 128 will be dimmed to 24 in complete darkness (lux 0)
    // which will be APA102 gamma corrected to 1.
    float dim = (3.0f + std::min(13.0f, std::log2(1.0f + getLux()))) / 16.0f;

    Rendering rendering(
	smoothTime.millisecondsSinceTwelveLocaltime(),
	millisecondsPerSecond,
	dim,
	hourGlow,
	2 * hourWidth	+ 0.5f,
	2 * minuteWidth + 0.5f,
	2 * secondWidth + 0.5f,
	function::compose<float, float, LED<int>>(
	    Ramp<LED<int>>(LED<int>(hourTail), LED<int>(hourMean)),
	    Bell<float>(hourWidth, 0.0f, 1.0f)
	),
	function::compose<float, float, LED<int>>(
	    Ramp<LED<int>>(LED<int>(minuteTail), LED<int>(minuteMean)),
	    Bell<float>(minuteWidth, 0.0f, 1.0f)
	),
	function::compose<float, float, LED<int>>(
	    Ramp<LED<int>>(LED<int>(secondTail), LED<int>(secondMean)),
	    Bell<float>(secondWidth, 0.0f, 1.0f)
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

ArtTask::ArtTask(
    SPI::Bus const *		spiBus1,
    SPI::Bus const *		spiBus2,
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ::ArtTask		("clockArtTask", 5, 16384, 1,
			spiBus1, spiBus2, getLux_, keyValueBroker_)
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
    ESP_LOGI(name, "~Clock::ArtTask");
}

}
