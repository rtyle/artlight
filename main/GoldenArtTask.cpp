#include <random>
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

static float constexpr pi	{std::acos(-1.0f)};
static float constexpr tau	{2.0f * pi};

static float constexpr phi	{(1.0f + std::sqrt(5.0f)) / 2.0f};
static float constexpr sqrt2	{std::sqrt(2.0f)};

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

static size_t constexpr ledCount {1024};

void GoldenArtTask::update_() {
    static size_t constexpr dialCount	{3};

    uint64_t const microsecondsSinceBoot {get_time_since_boot()};

    static LEDI const black {0, 0, 0};
    Blend<LEDI> const blend[dialCount] {
	{black, color[0]},
	{black, color[1]},
	{black, color[2]},
    };

    static float constexpr scale[dialCount] {
	phi	/ 1.5f,		//  > 1.0
	sqrt2	/ 1.5f,		//  < 1.0
	1.5f	/ 1.5f,		// == 1.0
    };

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
