#pragma once

#include <functional>

#include "APA102.h"
#include "ArtTask.h"
#include "Button.h"

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask Ring::ArtTask
#endif

namespace Ring {

class ArtTask : public ::ArtTask {
private:
    Button			button[4];
    void update();

public:
    ArtTask(
	SPI::Bus const *	spiBus1,
	SPI::Bus const *	spiBus2,
	ObservablePin		(&pin)[4],
	LEDC::Channel		(&ledChannel)[3][3],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    /* virtual */ void run() override;

    /* virtual */ ~ArtTask() override;
};

}
