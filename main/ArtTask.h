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

    float				hourWidth;
    uint32_t				hourMean;
    uint32_t				hourTail;
    KeyValueBroker::Observer const	hourWidthObserver;
    KeyValueBroker::Observer const	hourMeanObserver;
    KeyValueBroker::Observer const	hourTailObserver;

    float				minuteWidth;
    uint32_t				minuteMean;
    uint32_t				minuteTail;
    KeyValueBroker::Observer const	minuteWidthObserver;
    KeyValueBroker::Observer const	minuteMeanObserver;
    KeyValueBroker::Observer const	minuteTailObserver;

    float				secondWidth;
    uint32_t				secondMean;
    uint32_t				secondTail;
    KeyValueBroker::Observer const	secondWidthObserver;
    KeyValueBroker::Observer const	secondMeanObserver;
    KeyValueBroker::Observer const	secondTailObserver;

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
