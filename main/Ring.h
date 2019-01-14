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

struct TP {
public:
    float const	t;
    float const	p;
    TP(float t_, float p_) : t(t_), p(p_) {}
};

class ArtTask : public ::ArtTask {
private:
    std::function<APA102::LED<>(TP)> art;

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
