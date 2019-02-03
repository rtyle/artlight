#include <stdlib.h>
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

    getLux		(getLux_),

    keyValueBroker	(keyValueBroker_),

    gammaEncode		(1.2f),

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

	smoothTime	("smoothTime", 4096)
{}
