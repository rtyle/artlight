#pragma once

#include "APA102.h"
#include "AsioTask.h"
#include "GammaEncode.h"
#include "InRing.h"
#include "KeyValueBroker.h"
#include "SPI.h"
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
    struct Shape {
    private:
	static char const * const string[];
    public:
	enum Value {bell, wave, bloom} value;
	Shape(Value);
	Shape(char const *);
	char const * toString() const;
    };

    void widthObserved(size_t index, char const * value);
    void colorObserved(size_t index, char const * value);
    void shapeObserved(size_t index, char const * value);

protected:
    SPI::Device const			spiDevice[2];

    std::function<float()> const	getLux;
    KeyValueBroker &			keyValueBroker;

    KeyValueBroker::Observer const	timezoneObserver;

    float				width[3];
    APA102::LED<>			color[3];
    Shape				shape[3];

    KeyValueBroker::Observer const	widthObserver[3];
    KeyValueBroker::Observer const	colorObserver[3];
    KeyValueBroker::Observer const	shapeObserver[3];

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

	SPI::Bus const		(&spiBus)[2],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);
};
