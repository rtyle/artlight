#pragma once

#include "APA102.h"
#include "AsioTask.h"
#include "GammaEncode.h"
#include "KeyValueBroker.h"
#include "LEDC.h"
#include "Pin.h"
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
    ObservablePin			(&pin)[4];
    LEDC::Channel			(&ledChannel)[3][3];

    std::function<float()> const	getLux;
    KeyValueBroker &			keyValueBroker;

    KeyValueBroker::Observer const	timezoneObserver;

    float				aWidth;
    APA102::LED<>			aColor;
    APA102::LED<>			aFades;
    std::string				aShape;
    KeyValueBroker::Observer const	aWidthObserver;
    KeyValueBroker::Observer const	aColorObserver;
    KeyValueBroker::Observer const	aFadesObserver;
    KeyValueBroker::Observer const	aShapeObserver;

    float				bWidth;
    APA102::LED<>			bColor;
    APA102::LED<>			bFades;
    std::string				bShape;
    KeyValueBroker::Observer const	bWidthObserver;
    KeyValueBroker::Observer const	bColorObserver;
    KeyValueBroker::Observer const	bFadesObserver;
    KeyValueBroker::Observer const	bShapeObserver;

    float				cWidth;
    APA102::LED<>			cColor;
    APA102::LED<>			cFades;
    std::string				cShape;
    KeyValueBroker::Observer const	cWidthObserver;
    KeyValueBroker::Observer const	cColorObserver;
    KeyValueBroker::Observer const	cFadesObserver;
    KeyValueBroker::Observer const	cShapeObserver;

    enum struct Range {clip, normalize}	range;
    KeyValueBroker::Observer const	rangeObserver;
    enum struct Dim {automatic, manual}	dim;
    KeyValueBroker::Observer const	dimObserver;
    unsigned				dimLevel;
    KeyValueBroker::Observer const	dimLevelObserver;

    GammaEncode				gammaEncode;
    unsigned				gamma;
    KeyValueBroker::Observer const	gammaObserver;

    SmoothTime				smoothTime;

    void fadesObserver(
	APA102::LED<> & fades, char const * key, APA102::LED<int> value);

    ArtTask(
	char const *		name,
	UBaseType_t		priority,
	size_t			stackSize,
	BaseType_t		core,

	SPI::Bus const *	spiBus1,
	SPI::Bus const *	spiBus2,
	ObservablePin		(&pin)[4],
	LEDC::Channel		(&ledChannel)[3][3],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);
};
