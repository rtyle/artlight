#include <cstdlib>
extern "C" int setenv(char const *, char const *, int);

#include "APA102.h"
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
    aMean		(0u),
    aTail		(0u),
    aWidthObserver(keyValueBroker, "aWidth", "4",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		aWidth = width;
	    });
	}),
    aMeanObserver(keyValueBroker, "aMean", "#ff0000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		aMean = led;
	    });
	}),
    aTailObserver(keyValueBroker, "aTail", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		aTail = led;
	    });
	}),

    bWidth		(1.0f),
    bMean		(0u),
    bTail		(0u),
    bWidthObserver(keyValueBroker, "bWidth", "4",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		bWidth = width;
	    });
	}),
    bMeanObserver(keyValueBroker, "bMean", "#0000ff",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		bMean = led;
	    });
	}),
    bTailObserver(keyValueBroker, "bTail", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		bTail = led;
	    });
	}),

    cWidth		(1.0f),
    cMean		(0u),
    cTail		(0u),
    cWidthObserver(keyValueBroker, "cWidth", "2",
	[this](char const * widthObserved){
	    float width = fromString<float>(widthObserved);
	    io.post([this, width](){
		cWidth = width;
	    });
	}),
    cMeanObserver(keyValueBroker, "cMean", "#ffff00",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		cMean = led;
	    });
	}),
    cTailObserver(keyValueBroker, "cTail", "#000000",
	[this](char const * color){
	    LED<> led(color);
	    io.post([this, led](){
		cTail = led;
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
