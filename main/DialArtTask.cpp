#include "DialArtTask.h"

#include "fromString.h"

char const * const DialArtTask::Shape::string[] {"bell", "wave", "bloom"};
DialArtTask::Shape::Shape(Value value_) : value(value_) {}
DialArtTask::Shape::Shape(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return bell;
    }()
) {}
char const * DialArtTask::Shape::toString() const {
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

void DialArtTask::widthObserved(size_t index, char const * value_) {
    float value = fromString<float>(value_);
    if (0.0f <= value && value <= 64.0f) {
	io.post([this, index, value](){
	    width[index] = value;
	});
    }
}

void DialArtTask::colorObserved(size_t index, char const * value_) {
    if (APA102::isColor(value_)) {
	APA102::LED<> value(value_);
	io.post([this, index, value](){
	    color[index] = value;
	});
    }
}

void DialArtTask::shapeObserved(size_t index, char const * value_) {
    Shape value(value_);
    io.post([this, index, value](){
	shape[index] = value;
    });
}

DialArtTask::DialArtTask(
    char const *		name,
    UBaseType_t			priority,
    size_t			stackSize,
    BaseType_t			core,

    SPI::Bus const		(&spiBus)[2],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_,
    size_t			smoothTimeStepCount)
:
    ArtTask		{name, priority, stackSize, core,
			getLux_, keyValueBroker_, smoothTimeStepCount},

    spiDevice {
	{&spiBus[0], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)			// no chip select
	    .queue_size_(1)
	},
	{&spiBus[1], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)			// no chip select
	    .queue_size_(1)
	},
    },

    width {},
    color {},
    shape {
	Shape::Value::bell,
	Shape::Value::bell,
	Shape::Value::bell,
    },

    widthObserver {
	{keyValueBroker, widthKey[0], "8",
	    [this](char const * value) {widthObserved(0, value);}},
	{keyValueBroker, widthKey[1], "8",
	    [this](char const * value) {widthObserved(1, value);}},
	{keyValueBroker, widthKey[2], "4",
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
    }
{}
