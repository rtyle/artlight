#pragma once

#include "AsioTask.h"
#include "GammaEncode.h"
#include "KeyValueBroker.h"
#include "SmoothTime.h"

/// An ArtTask is an abstract base class for an implementation that
///	* uses devices on the system's SPI buses
///	* adjusts its presentation based on current ambient lighting levels
///	* adjusts its presentation based on updated values for interesting keys

class ArtTask : public AsioTask {
public:
    struct Dim {
    private:
	static char const * const string[];
    public:
	enum Value {automatic, manual} value;
	Dim(Value);
	Dim(char const *);
	char const * toString() const;
    };
    struct Range {
    private:
	static char const * const string[];
    public:
	enum Value {clip, normalize} value;
	Range(Value);
	Range(char const *);
	char const * toString() const;
    };

protected:
    std::function<float()> const	getLux;
    KeyValueBroker &			keyValueBroker;

    KeyValueBroker::Observer const	timezoneObserver;

    Range				range;
    KeyValueBroker::Observer const	rangeObserver;
    Dim					dim;
    KeyValueBroker::Observer const	dimObserver;
    unsigned				dimLevel;
    KeyValueBroker::Observer const	dimLevelObserver;

    GammaEncode				gammaEncode;
    unsigned				gamma;
    KeyValueBroker::Observer const	gammaObserver;

    SmoothTime				smoothTime;

    ArtTask(
	char const *		name,
	UBaseType_t		priority,
	size_t			stackSize,
	BaseType_t		core,

	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker,
	size_t			smoothTimeStepCount);
};
