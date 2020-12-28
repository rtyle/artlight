#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <random>
#include <set>
#include <sstream>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "clip.h"
#include "fromString.h"
#include "Blend.h"
#include "GoldenArtTask.h"
#include "Curve.h"
#include "PerlinNoise.hpp"
#include "Pulse.h"
#include "Timer.h"

extern "C" uint64_t get_time_since_boot();

using APA102::LED;
using LEDI = APA102::LED<int>;

float constexpr phi	{(1.0f + std::sqrt(5.0f)) / 2.0f};
float constexpr pi	{std::acos(-1.0f)};
float constexpr tau	{2.0f * pi};

namespace {
    struct Cartesian;
    struct Polar {
	float rho;
	float theta;
	static float normalize(float theta) {
	    float i;
	    float f = std::modf(theta / tau, &i);
	    if (0 > f) f += 1;
	    return tau * f;
	}
	Polar() = default;
	Polar(float rho_, float theta_) : rho{rho_}, theta{normalize(theta_)} {}
	Polar(Cartesian const &);
    };
    struct Cartesian {
	float x;
	float y;
	Cartesian() = default;
	Cartesian(float x_, float y_) : x{x_}, y{y_} {}
	Cartesian(Polar const & _)
	: Cartesian{_.rho * std::cos(_.theta), _.rho * std::sin(_.theta)} {}
    };
    Polar::Polar(Cartesian const & _)
    : Polar{std::sqrt(_.x * _.x + _.y * _.y), std::atan2(_.y, _.x)} {}
    template<uint16_t size>
    struct PolarEndSortTheta {
        std::array <int16_t, size> order;
        PolarEndSortTheta(Polar const * polarEnd) {
            {int16_t i = 0; for (auto & e: order) {e = --i;}}
            std::sort(order.begin(), order.end(), [polarEnd](int16_t const & a, int16_t const & b){
                return polarEnd[a].theta < polarEnd[b].theta;
            });
        }
        void copy(std::array<int16_t, size> & to) const {
            std::copy(order.begin(), order.end(), to.begin());
        }
    };



    template<uint16_t size>
    struct Head {
	std::array<Polar, size> polar;
	std::array<Cartesian, size> cartesian;
        std::array<int16_t, 144> rim144;
        std::array<int16_t,  89> rim89;
        std::array<int16_t,  55> rim55;
	std::array<uint16_t, size> order;
	Head() {
	    // calculate polar and cartesian coordinates
	    {uint16_t i = 0; for (auto & _: polar) {
		cartesian[i] = Cartesian{
		    _ = Polar{
			18.0f * std::sqrt(1.0f + i),
			static_cast<float>(i) * tau / phi
		    }
		};
	    ++i;}}
            // orders along the rim
            Polar const * polarEnd{&polar[polar.size()]};
            PolarEndSortTheta<144>(polarEnd).copy(rim144);
            PolarEndSortTheta< 89>(polarEnd).copy(rim89);
            PolarEndSortTheta< 55>(polarEnd).copy(rim55);
	    // find a spiral path
	    std::array<uint16_t, size> spiral;
	    std::set<uint16_t> remaining;
	    for (uint16_t i = 0; i < size; ++i) remaining.insert(i);
	    uint16_t * last = &spiral[size];
	    remaining.erase(remaining.find(*--last = size - 1));
	    while (!remaining.empty()) {
		// spin the remaining around the last,
		// and find the smallest angle amongst them
		Polar const & a{polar[*last]};
		float smallest{tau};
		std::array<Polar, size> spin;
		for (auto i: remaining) {
		    Polar const & b{polar[i]};
		    Cartesian c{Polar{b.rho, b.theta - a.theta}};
		    Polar p{Cartesian{c.x - a.rho, c.y}};
		    spin[i] = p;
		    if (smallest > p.theta) smallest = p.theta;
		}
		// next is the nearest within an acceptable angle
		float acceptable{smallest + tau / 8};
		float nearest{std::numeric_limits<float>::max()};
		uint16_t next{size};
		for (auto i: remaining) {
		    Polar & p = spin[i];
		    if (acceptable >= p.theta && nearest > p.rho) {
			nearest = p.rho;
			next = i;
		    }
		}
		assert(std::numeric_limits<float>::max() != nearest);
		remaining.erase(remaining.find(*--last = next));
	    }
	    // invert the spiral path for order
	    for (auto & i: spiral) {
		order[spiral[i]] = i;
	    }
	}
    };
}
size_t constexpr ledCount {1024};
static Head<ledCount> head;

static unsigned constexpr millisecondsPerSecond	{1000u};
static unsigned constexpr microsecondsPerSecond	{1000000u};

static Pulse hourPulse	{12};
static Pulse minutePulse{60};
static Pulse secondPulse{60};

static SawtoothCurve inMinuteOf{0.0f, 60.0f};
static SawtoothCurve inHourOf  {0.0f, 60.0f * 60.0f};
static SawtoothCurve inDayOf   {0.0f, 60.0f * 60.0f * 12.0f};	/// 12 hour clock

char const * const GoldenArtTask::Mode::string[]
    {"clock", "slide", "spin"};
GoldenArtTask::Mode::Mode(Value value_) : value(value_) {}
GoldenArtTask::Mode::Mode(char const * value) : value(
    [value](){
	size_t i {0};
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return clock;
    }()
) {}
char const * GoldenArtTask::Mode::toString() const {
    return string[value];
}


void GoldenArtTask::update_() {
    uint64_t const microsecondsSinceBoot {get_time_since_boot()};

    // construct static PerlinNoise objects
    static std::mt19937 rng;
    static PerlinNoise perlinNoise[] {rng, rng, rng, rng};
    // Perlin noise repeats every 256 units.
    static unsigned constexpr perlinNoisePeriod {256};
    static uint64_t constexpr perlinNoisePeriodMicroseconds
	{perlinNoisePeriod * microsecondsPerSecond};
    // Perlin noise at an integral grid point is 0.
    // To avoid this, cut between them.

    APA102::Message<1> message;

    message.encodings[0] = {[microsecondsSinceBoot]() -> uint32_t {
	static int constexpr max {64};
	static int constexpr octaves {1};
	static float constexpr mid {0.5f};
	float const z {(microsecondsSinceBoot % perlinNoisePeriodMicroseconds)
	    / static_cast<float>(microsecondsPerSecond)};
	return LEDI(
	    max * perlinNoise[0].octaveNoise0_1(mid, mid, z, octaves),
	    max * perlinNoise[1].octaveNoise0_1(mid, mid, z, octaves),
	    max * perlinNoise[2].octaveNoise0_1(mid, mid, z, octaves)
    );}()};

    // SPI::Transaction constructor queues the message.
    // SPI::Transaction destructor waits for result.
    {
	SPI::Transaction transaction0(spiDevice[0], SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
	SPI::Transaction transaction1(spiDevice[1], SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
    }
}

void GoldenArtTask::update() {
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

GoldenArtTask::GoldenArtTask(
    KeyValueBroker &	keyValueBroker)
:
    AsioTask		{"GoldenArtTask", 5, 0x10000, 1},
    TimePreferences	{io, keyValueBroker, 512},
    DialPreferences	{io, keyValueBroker},

    tinyPicoLedPower {GPIO_NUM_13, GPIO_MODE_OUTPUT},

    spiBus {
	{HSPI_HOST, SPI::Bus::Config()
	    .mosi_io_num_(2)
	    .sclk_io_num_(12),
	0},
	{VSPI_HOST, SPI::Bus::Config()
	    .mosi_io_num_(SPI::Bus::VspiConfig.mosi_io_num)
	    .sclk_io_num_(SPI::Bus::VspiConfig.sclk_io_num)
	    .max_transfer_sz_(APA102::messageBits(ledCount)),
	1},
    },

    spiDevice {
	{&spiBus[0], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_( 100000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)		// no chip select
	    .queue_size_(1)
	},
	{&spiBus[1], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)		// no chip select
	    .queue_size_(1)
	},
    },

    mode(Mode::clock),
    modeObserver(keyValueBroker, "mode", mode.toString(),
	[this](char const * value){
	    Mode mode_(value);
	    io.post([this, mode_](){
		mode = mode_;
	    });
	}),

    updated(0)
{
    tinyPicoLedPower.set_level(0);	// high side switch, low (0) turns it on
}

void GoldenArtTask::run() {
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

GoldenArtTask::~GoldenArtTask() {
    stop();
    ESP_LOGI(name, "~GoldenArtTask");
}
