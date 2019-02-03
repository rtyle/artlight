#pragma once

#include "AsioTask.h"
#include "GammaEncode.h"
#include "KeyValueBroker.h"
#include "SPI.h"
#include "SmoothTime.h"

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

    GammaEncode const			gammaEncode;

    KeyValueBroker::Observer const	timezoneObserver;

    float				aWidth;
    uint32_t				aMean;
    uint32_t				aTail;
    KeyValueBroker::Observer const	aWidthObserver;
    KeyValueBroker::Observer const	aMeanObserver;
    KeyValueBroker::Observer const	aTailObserver;

    float				bWidth;
    uint32_t				bMean;
    uint32_t				bTail;
    KeyValueBroker::Observer const	bWidthObserver;
    KeyValueBroker::Observer const	bMeanObserver;
    KeyValueBroker::Observer const	bTailObserver;

    float				cWidth;
    uint32_t				cMean;
    uint32_t				cTail;
    KeyValueBroker::Observer const	cWidthObserver;
    KeyValueBroker::Observer const	cMeanObserver;
    KeyValueBroker::Observer const	cTailObserver;

    SmoothTime				smoothTime;

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
