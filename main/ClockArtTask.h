#ifndef ClockArtTask_h_
#define ClockArtTask_h_

#include <ctime>
#include <functional>
#include <list>

#include "AsioTask.h"
#include "SPI.h"

class ClockArtTask : public AsioTask {
private:
    SPI::Device const			spiDevice1;
    SPI::Device const			spiDevice2;
    std::function<float()> const	getLux;

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
	std::function<float()>	getLux);

    /* virtual */ void run() override;

    /* virtual */ ~ClockArtTask() override;
};

#endif
