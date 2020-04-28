#include <cstdlib>
extern "C" int setenv(char const *, char const *, int);

#include "ArtTask.h"

char const * const ArtTask::Dim::string[] {"automatic", "manual",};
ArtTask::Dim::Dim(Value value_) : value(value_) {}
ArtTask::Dim::Dim(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return automatic;
    }()
) {}
char const * ArtTask::Dim::toString() const {
    return string[value];
}

char const * const ArtTask::Range::string[] {"clip", "normalize",};
ArtTask::Range::Range(Value value_) : value(value_) {}
ArtTask::Range::Range(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return clip;
    }()
) {}
char const * ArtTask::Range::toString() const {
    return string[value];
}

ArtTask::ArtTask(
    char const *		name,
    UBaseType_t			priority,
    size_t			stackSize,
    BaseType_t			core,

    SPI::Bus const		(&spiBus)[2],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_,
    size_t			smoothTimeStepCount,
    uint8_t			(&spiMode)[2])
:
    AsioTask		(name, priority, stackSize, core),

    spiDevice {
	{&spiBus[0], SPI::Device::Config()
	    .mode_(spiMode[0])
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)		// no chip select
	    .queue_size_(1)
	},
	{&spiBus[1], SPI::Device::Config()
	    .mode_(spiMode[1])
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)		// no chip select
	    .queue_size_(1)
	},
    },

    getLux		(getLux_),

    keyValueBroker	(keyValueBroker_),

    // timezone affects our notion of the localtime we present
    // forward a copy of any update to our task to make a synchronous change
    timezoneObserver(keyValueBroker, "timezone", CONFIG_TIME_ZONE,
	    [this](char const * timezone){
		std::string timezoneCopy(timezone);
		io.post([timezoneCopy](){
		    setenv("TZ", timezoneCopy.c_str(), true);
		});
	    }),

    range(Range::clip),
    rangeObserver(keyValueBroker, "range", range.toString(),
	[this](char const * value){
	    Range range_(value);
	    io.post([this, range_](){
		range = range_;
	    });
	}),
    dim(Dim::automatic),
    dimObserver(keyValueBroker, "dim", dim.toString(),
	[this](char const * value){
	    Dim dim_ (value);
	    io.post([this, dim_](){
		dim = dim_;
	    });
	}),
    dimLevel(16),
    dimLevelObserver(keyValueBroker, "dimLevel", "16",
	[this](char const * value){
	    unsigned const level = std::strtoul(value, nullptr, 10);
		if (3 <= level && level <= 16) {
		io.post([this, level](){
		    dimLevel = level;
		});
	    }
	}),
    gammaEncode(2.0),
    gamma(20),
    gammaObserver(keyValueBroker, "gamma", "20",
	[this](char const * value){
	    unsigned const level = std::strtoul(value, nullptr, 10);
	    if (5 <= level && level <= 30) {
		io.post([this, level](){
		    gamma = level;
		    gammaEncode.gamma(gamma / 10.0f);
		});
	    }
	}),

    smoothTime	("smoothTime", smoothTimeStepCount)
{}
