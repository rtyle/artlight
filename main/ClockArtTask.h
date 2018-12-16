#ifndef ClockArtTask_h_
#define ClockArtTask_h_

#include <ctime>
#include <functional>
#include <list>

#include "AsioTask.h"
#include "KeyValueBroker.h"
#include "SPI.h"

class ClockArtTask : public AsioTask {
private:
    SPI::Device const			spiDevice1;
    SPI::Device const			spiDevice2;
    std::function<float()> const	getLux;
    KeyValueBroker &			keyValueBroker;

    KeyValueBroker::Observer const	timezoneObserver;

    float				hourShape;
    uint32_t				hourMean;
    uint32_t				hourTail;
    uint32_t				hourGlow;
    KeyValueBroker::Observer const	hourShapeObserver;
    KeyValueBroker::Observer const	hourMeanObserver;
    KeyValueBroker::Observer const	hourTailObserver;
    KeyValueBroker::Observer const	hourGlowObserver;

    float				minuteShape;
    uint32_t				minuteMean;
    uint32_t				minuteTail;
    KeyValueBroker::Observer const	minuteShapeObserver;
    KeyValueBroker::Observer const	minuteMeanObserver;
    KeyValueBroker::Observer const	minuteTailObserver;

    float				secondShape;
    uint32_t				secondMean;
    uint32_t				secondTail;
    KeyValueBroker::Observer const	secondShapeObserver;
    KeyValueBroker::Observer const	secondMeanObserver;
    KeyValueBroker::Observer const	secondTailObserver;

    class SmoothTime {
    private:
	char const *	name;
	size_t const	stepCount;
	size_t		stepLeft;
	float		stepProduct;
	float		stepFactor;
	int64_t		lastBootTime;
    public:
	SmoothTime(char const * name, size_t count);
	int64_t microsecondsSinceEpoch();
	uint32_t millisecondsSinceTwelveLocaltime();
    } smoothTime;

    void update();

public:
    ClockArtTask(
	SPI::Bus const *	spiBus1,
	SPI::Bus const *	spiBus2,
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    /* virtual */ void run() override;

    /* virtual */ ~ClockArtTask() override;
};

#endif
