#include <cstdlib>
extern "C" int setenv(char const *, char const *, int);

#include "ArtTask.h"

#include "fromString.h"

void ArtTask::fadesObserver(
    APA102::LED<> &	fades,
    char const *	key,
    APA102::LED<int>	value)
{
    static int constexpr ceiling = 0x30;
    int sum = value.sum();
    if (ceiling < sum) {
	std::string const newValue(APA102::LED<>(value * ceiling / sum));
	keyValueBroker.publish(key, newValue.c_str());
    } else {
	io.post([&fades, value]() {
	    fades = value;
	});
    }

}

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

    aWidth		(1.0f),
    aColor		(0u),
    aFades		(0u),
    aShape		(Shape::Value::bell),
    aWidthObserver(keyValueBroker, "aWidth", "4",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		aWidth = width;
	    });
	}),
    aColorObserver(keyValueBroker, "aColor", "#ff0000",
	[this](char const * color){
	    APA102::LED<> led(color);
	    io.post([this, led](){
		aColor = led;
	    });
	}),
    aFadesObserver(keyValueBroker, "aFades", "#000000",
	[this](char const * color){
	    fadesObserver(aFades, "aFades", color);
	}),
    aShapeObserver(keyValueBroker, "aShape", aShape.toString(),
	[this](char const * value){
	    Shape shape(value);
	    io.post([this, shape](){
		aShape = shape;
	    });
	}),

    bWidth		(1.0f),
    bColor		(0u),
    bFades		(0u),
    bShape		(Shape::Value::bell),
    bWidthObserver(keyValueBroker, "bWidth", "4",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		bWidth = width;
	    });
	}),
    bColorObserver(keyValueBroker, "bColor", "#0000ff",
	[this](char const * color){
	    APA102::LED<> led(color);
	    io.post([this, led](){
		bColor = led;
	    });
	}),
    bFadesObserver(keyValueBroker, "bFades", "#000000",
	[this](char const * color){
	    fadesObserver(bFades, "bFades", color);
	}),
    bShapeObserver(keyValueBroker, "bShape", bShape.toString(),
	[this](char const * value){
	    Shape shape(value);
	    io.post([this, shape](){
		bShape = shape;
	    });
	}),

    cWidth		(1.0f),
    cColor		(0u),
    cFades		(0u),
    cShape		(Shape::Value::bell),
    cWidthObserver(keyValueBroker, "cWidth", "2",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		cWidth = width;
	    });
	}),
    cColorObserver(keyValueBroker, "cColor", "#ffff00",
	[this](char const * color){
	    APA102::LED<> led(color);
	    io.post([this, led](){
		cColor = led;
	    });
	}),
    cFadesObserver(keyValueBroker, "cFades", "#000000",
	[this](char const * color){
	    fadesObserver(cFades, "cFades", color);
	}),
    cShapeObserver(keyValueBroker, "cShape", cShape.toString(),
	[this](char const * value){
	    Shape shape(value);
	    io.post([this, shape](){
		cShape = shape;
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
