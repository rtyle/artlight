#pragma once

#include <functional>

#include "APA102.h"
#include "ArtTask.h"

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask Ring::ArtTask
#endif

namespace Ring {

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
