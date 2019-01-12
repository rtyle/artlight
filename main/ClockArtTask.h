#pragma once

#include "ArtTask.h"

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask ClockArtTask
#endif

class ClockArtTask : public ArtTask {
private:
    KeyValueBroker::Observer const	timezoneObserver;

    float				hourWidth;
    uint32_t				hourMean;
    uint32_t				hourTail;
    uint32_t				hourGlow;
    KeyValueBroker::Observer const	hourWidthObserver;
    KeyValueBroker::Observer const	hourMeanObserver;
    KeyValueBroker::Observer const	hourTailObserver;
    KeyValueBroker::Observer const	hourGlowObserver;

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
