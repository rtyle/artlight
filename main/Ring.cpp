#include <esp_log.h>

#include "Ring.h"
#include "Timer.h"

#include "function.h"

using APA102::LED;
using function::compose;
using function::combine;

namespace Ring {

static float const pi = std::acos(-1.0f);

template <typename T>
class Dim {
private:
    float	dim;
public:
    Dim(float dim_) : dim(dim_) {}
    T operator()(T t) {return t * dim;}
};

/// Sawtooth is a function object defined by its period.
/// Its output ranges from 0 to <1.
class Sawtooth {
private:
    float period;
public:
    Sawtooth(float period_ = 1.0f) : period(period_) {}
    float operator()(float x) {
	float y = std::modf(x / period, &x);
	return y;
    }
};

static Sawtooth inSecondOf(1.0f);
static Sawtooth inMinuteOf(60.0f);
static Sawtooth inHourOf  (60.0f * 60.0f);
static Sawtooth inDayOf   (60.0f * 60.0f * 12.0f);

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
struct At {
public:
    float const	place;
    float const	time;
    At(float place_, float time_) : place(place_), time(time_) {}
    float center(Moving const & moving) const {
	return place - moving.center - time * moving.speed;
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
	float frequency_	= 1.0f)
    :
	Moving		(center, speed),
	frequency	(frequency_)
    {}
    float operator()(At at) const {
	return (1.0f + std::cos(2.0f * pi * frequency * at.center(*this)))
	    / 2.0f;
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

static LED<> gamma(LED<> led) {return led.gamma();}

void ArtTask::update() {
    // dim factor is calculated as a function of the current ambient lux.
    // this will range from 3/16 to 16/16 with the numerator increasing by
    // 1 as the lux doubles up until 2^13 (~full daylight, indirect sun).
    // an LED value of 128 will be dimmed to 24 in complete darkness (lux 0)
    // which will be APA102 gamma corrected to 1.
    Dim<LED<>> dim((3.0f + std::min(13.0f, std::log2(1.0f + getLux()))) / 16.0f);

    static size_t constexpr circumference = 144;
    APA102::Message<circumference> message;

    Ramp<LED<unsigned>> isr(LED<int>(0, 0, 0), LED<int>(128, 128, 128));
    Ramp<LED<unsigned>> imr(LED<int>(0, 0, 0), LED<int>(255, 255,   0));
    Ramp<LED<unsigned>> ihr(LED<int>(0, 0, 0), LED<int>(  0,   0, 255));
    Ramp<LED<unsigned>> idr(LED<int>(0, 0, 0), LED<int>(255,   0,   0));

    Bell iss(0.0f, 1.0f, 1.0f / circumference);
    Bell ims(0.0f, 1.0f, 2.0f / circumference);
    Bell ihs(0.0f, 1.0f, 4.0f / circumference);
    Bell ids(0.0f, 1.0f, 8.0f / circumference);

    auto isa([isr, iss](At at){return isr(iss(at));});
    auto ima([imr, ims](At at){return imr(ims(at));});
    auto iha([ihr, ihs](At at){return ihr(ihs(at));});
    auto ida([idr, ids](At at){return idr(ids(at));});

    float time =
	static_cast<float>(smoothTime.millisecondsSinceTwelveLocaltime()) / 1000.0f
	// esp_timer_get_time() / 1000000.0f
    ;
    float is(inSecondOf(time));
    float im(inMinuteOf(time));
    float ih(inHourOf  (time));
    float id(inDayOf   (time));

    size_t i = 0;
    for (auto & e: message.encodings) {
	float place = static_cast<float>(i) / circumference;
	e = gamma(dim([place, isa, ima, iha, ida, is, im, ih, id](){
	    return LED<unsigned>(255, 255, 255).minByPart(
		  isa(At(place, is))
		+ ima(At(place, im))
		+ iha(At(place, ih))
		+ ida(At(place, id))
		);
	}()));
	i++;
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
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ::ArtTask		("ringArtTask", 5, 16384, 1,
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
    ESP_LOGI(name, "~Ring::ArtTask");
}

}