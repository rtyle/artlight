#pragma once

#include "ArtTask.h"

/// An ArtLight has an ArtTask.
/// The ArtLight implementation is a factory
/// that is configured to choose the ArtTask implementation.
class ArtLight {
public:
    ArtTask * const		artTask;

    ArtLight(
	SPI::Bus const *	spiBus1,
	SPI::Bus const *	spiBus2,
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    ~ArtLight();
};
