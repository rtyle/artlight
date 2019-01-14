#pragma once

#include "AsioTask.h"
#include "KeyValueBroker.h"
#include "SPI.h"

/// An ArtTask is an abstract base class for an implementation that
///	* uses devices on the system's SPI buses
///	* adjusts its presentation based on current ambient lighting levels
///	* adjusts its presentation based on updated values for interesting keys

class ArtTask : public AsioTask {
protected:
    SPI::Device const			spiDevice1;
    SPI::Device const			spiDevice2;
    std::function<float()> const	getLux;
    KeyValueBroker &			keyValueBroker;
    KeyValueBroker::Observer const	timezoneObserver;

    ArtTask(
	char const *		name,
	UBaseType_t		priority,
	size_t			stackSize,
	BaseType_t		core,

	SPI::Bus const *	spiBus1,
	SPI::Bus const *	spiBus2,
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);
};
