#include <cstdlib>
extern "C" int setenv(char const *, char const *, int);

#include "ArtTask.h"

#include "fromString.h"

using APA102::LED;

ArtTask::ArtTask(
    char const *		name,
    UBaseType_t			priority,
    size_t			stackSize,
    BaseType_t			core,

    SPI::Bus const *		spiBus1,
    SPI::Bus const *		spiBus2,
    ObservablePin		(&pin_)[4],
    LEDC::Channel		(&ledChannel_)[3][3],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    AsioTask		(name, priority, stackSize, core),

    spiDevice1		(spiBus1, SPI::Device::Config()
			    .mode_(APA102::spiMode)
			    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
			    .spics_io_num_(-1)		// no chip select
			    .queue_size_(1)
			),
    spiDevice2		(spiBus2, SPI::Device::Config()
			    .mode_(APA102::spiMode)
			    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
			    .spics_io_num_(-1)		// no chip select
			    .queue_size_(1)
			),

    pin(pin_),

    ledChannel(ledChannel_),

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
    aWidthObserver(keyValueBroker, "aWidth", "4",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		aWidth = width;
	    });
	}),
    aColorObserver(keyValueBroker, "aColor", "#ff0000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		aColor = led;
	    });
	}),
    aFadesObserver(keyValueBroker, "aFades", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		aFades = led;
	    });
	}),

    bWidth		(1.0f),
    bColor		(0u),
    bFades		(0u),
    bWidthObserver(keyValueBroker, "bWidth", "4",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		bWidth = width;
	    });
	}),
    bColorObserver(keyValueBroker, "bColor", "#0000ff",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		bColor = led;
	    });
	}),
    bFadesObserver(keyValueBroker, "bFades", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		bFades = led;
	    });
	}),

    cWidth		(1.0f),
    cColor		(0u),
    cFades		(0u),
    cWidthObserver(keyValueBroker, "cWidth", "2",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		cWidth = width;
	    });
	}),
    cColorObserver(keyValueBroker, "cColor", "#ffff00",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		cColor = led;
	    });
	}),
    cFadesObserver(keyValueBroker, "cFades", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		cFades = led;
	    });
	}),

    range(Range::clip),
    rangeObserver(keyValueBroker, "range", "_clip",
	[this](char const * value){
	    Range range_ = 0 == strcmp("_clip", value)
		? Range::clip
		: Range::normalize;
	    io.post([this, range_](){
		range = range_;
	    });
	}),
    dim(Dim::automatic),
    dimObserver(keyValueBroker, "dim", "_automatic",
	[this](char const * value){
	    Dim dim_ = 0 == strcmp("_automatic", value)
		? Dim::automatic
		: Dim::manual;
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
