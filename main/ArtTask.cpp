#include <stdlib.h>
extern "C" int setenv(char const *, char const *, int);

#include "APA102.h"
#include "ArtTask.h"

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

    // timezone affects our notion of the localtime we present
    // forward a copy of any update to our task to make a synchronous change
    timezoneObserver(keyValueBroker, "timezone", CONFIG_TIME_ZONE,
	    [this](char const * timezone){
		std::string timezoneCopy(timezone);
		io.post([timezoneCopy](){
		    setenv("TZ", timezoneCopy.c_str(), true);
		});
	    })
{}
