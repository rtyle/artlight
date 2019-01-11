#include "ArtLight.h"

// choose the ClockArtTask ArtTask implementation

#include "ClockArtTask.h"

ArtLight::ArtLight(
    SPI::Bus const *		spiBus1,
    SPI::Bus const *		spiBus2,
    std::function<float()>	getLux,
    KeyValueBroker &		keyValueBroker)
:
    artTask(new ClockArtTask(spiBus1, spiBus2, getLux, keyValueBroker))
{}

ArtLight::~ArtLight() {
    if (artTask) delete artTask;
}
