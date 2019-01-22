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

    gammaEncode		(2.0f),

    // timezone affects our notion of the localtime we present
    // forward a copy of any update to our task to make a synchronous change
    timezoneObserver(keyValueBroker, "timezone", CONFIG_TIME_ZONE,
	    [this](char const * timezone){
		std::string timezoneCopy(timezone);
		io.post([timezoneCopy](){
		    setenv("TZ", timezoneCopy.c_str(), true);
		});
	    }),

	hourWidth		(1.0f),
	hourMean		(0u),
	hourTail		(0u),
	hourGlow		(0u),
	hourWidthObserver(keyValueBroker, "hourWidth", "4",
	    [this](char const * widthObserved){
		float width = fromString<float>(widthObserved);
		io.post([this, width](){
		    hourWidth = width;
		});
	    }),
	hourMeanObserver(keyValueBroker, "hourMean", "#ff0000",
	    [this](char const * color){
		LED<> led(color);
		io.post([this, led](){
		    hourMean = led;
		});
	    }),
	hourTailObserver(keyValueBroker, "hourTail", "#000000",
	    [this](char const * color){
		LED<> led(color);
		io.post([this, led](){
		    hourTail = led;
		});
	    }),
	hourGlowObserver(keyValueBroker, "hourGlow", "#000000",
	    [this](char const * color){
		LED<unsigned> led(color);
		static unsigned constexpr brightest = 3 * 128;
		unsigned brightness = led.sum();
		if (brightest < brightness) {
		    std::string dimmed(LED<>(led * brightest / brightness));
		    keyValueBroker.publish("hourGlow", dimmed.c_str());
		} else {
		    io.post([this, led](){
			hourGlow = led;
		    });
		}
	    }),

	minuteWidth		(1.0f),
	minuteMean		(0u),
	minuteTail		(0u),
	minuteWidthObserver(keyValueBroker, "minuteWidth", "4",
	    [this](char const * widthObserved){
		float width = fromString<float>(widthObserved);
		io.post([this, width](){
		    minuteWidth = width;
		});
	    }),
	minuteMeanObserver(keyValueBroker, "minuteMean", "#0000ff",
	    [this](char const * color){
		LED<> led(color);
		io.post([this, led](){
		    minuteMean = led;
		});
	    }),
	minuteTailObserver(keyValueBroker, "minuteTail", "#000000",
	    [this](char const * color){
		LED<> led(color);
		io.post([this, led](){
		    minuteTail = led;
		});
	    }),

	secondWidth		(1.0f),
	secondMean		(0u),
	secondTail		(0u),
	secondWidthObserver(keyValueBroker, "secondWidth", "2",
	    [this](char const * widthObserved){
		float width = fromString<float>(widthObserved);
		io.post([this, width](){
		    secondWidth = width;
		});
	    }),
	secondMeanObserver(keyValueBroker, "secondMean", "#ffff00",
	    [this](char const * color){
		LED<> led(color);
		io.post([this, led](){
		    secondMean = led;
		});
	    }),
	secondTailObserver(keyValueBroker, "secondTail", "#000000",
	    [this](char const * color){
		LED<> led(color);
		io.post([this, led](){
		    secondTail = led;
		});
	    }),

	smoothTime	("smoothTime", 4096)
{}
