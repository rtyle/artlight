#pragma once

#include "ArtTask.h"
#include "SmoothTime.h"

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask Clock::ArtTask
#endif

namespace Clock {

class ArtTask : public ::ArtTask {
private:
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

    SmoothTime smoothTime;

    void update();

public:
    ArtTask(
	SPI::Bus const *	spiBus1,
	SPI::Bus const *	spiBus2,
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    /* virtual */ void run() override;

    /* virtual */ ~ArtTask() override;
};

}
