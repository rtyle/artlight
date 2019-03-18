#include <cstdlib>
extern "C" int setenv(char const *, char const *, int);

#include "ArtTask.h"

#include "fromString.h"

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

char const * const ArtTask::Shape::string[] {"bell", "wave",};
ArtTask::Shape::Shape(Value value_) : value(value_) {}
ArtTask::Shape::Shape(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return bell;
    }()
) {}
char const * ArtTask::Shape::toString() const {
    return string[value];
}

char const * const widthKey[] {
    "aWidth",
    "bWidth",
    "cWidth",
};
char const * const colorKey[] {
    "aColor",
    "bColor",
    "cColor",
};
char const * const shapeKey[] {
    "aShape",
    "bShape",
    "cShape",
};

void ArtTask::widthObserved(size_t index, char const * value_) {
    float value = fromString<float>(value_);
    io.post([this, index, value](){
	width[index] = value;
    });
}

void ArtTask::colorObserved(size_t index, char const * value_) {
    APA102::LED<> value(value_);
    io.post([this, index, value](){
	color[index] = value;
    });
}

void ArtTask::shapeObserved(size_t index, char const * value_) {
    Shape value(value_);
    io.post([this, index, value](){
	shape[index] = value;
    });
}

ArtTask::ArtTask(
    char const *		name,
    UBaseType_t			priority,
    size_t			stackSize,
    BaseType_t			core,

    SPI::Bus const		(&spiBus)[2],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    AsioTask		(name, priority, stackSize, core),

    spiDevice {
	{&spiBus[0], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
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

    width {},
    color {},
    shape {
	Shape::Value::bell,
	Shape::Value::bell,
	Shape::Value::bell,
    },

    widthObserver {
	{keyValueBroker, widthKey[0], "4",
	    [this](char const * value) {widthObserved(0, value);}},
	{keyValueBroker, widthKey[1], "4",
	    [this](char const * value) {widthObserved(1, value);}},
	{keyValueBroker, widthKey[2], "2",
	    [this](char const * value) {widthObserved(2, value);}},
    },

    colorObserver {
	{keyValueBroker, colorKey[0], "#ff0000",
	    [this](char const * value) {colorObserved(0, value);}},
	{keyValueBroker, colorKey[1], "#0000ff",
	    [this](char const * value) {colorObserved(1, value);}},
	{keyValueBroker, colorKey[2], "#ffff00",
	    [this](char const * value) {colorObserved(2, value);}},
    },

    shapeObserver {
	{keyValueBroker, shapeKey[0], shape[0].toString(),
	    [this](char const * value) {shapeObserved(0, value);}},
	{keyValueBroker, shapeKey[1], shape[1].toString(),
	    [this](char const * value) {shapeObserved(1, value);}},
	{keyValueBroker, shapeKey[2], shape[2].toString(),
	    [this](char const * value) {shapeObserved(2, value);}},
    },

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
	    unsigned dimLevel_ = std::strtoul(value, nullptr, 10);
	    io.post([this, dimLevel_](){
		dimLevel = dimLevel_;
	    });
	}),
    gammaEncode(2.0),
    gamma(20),
    gammaObserver(keyValueBroker, "gamma", "20",
	[this](char const * value){
	    int gamma_ = std::strtoul(value, nullptr, 10);
	    io.post([this, gamma_](){
		gamma = gamma_;
		gammaEncode.gamma(gamma / 10.0f);
	    });
	}),

    smoothTime	("smoothTime", 4096)
{}
