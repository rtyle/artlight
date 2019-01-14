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
