#pragma once

#include "ArtTask.h"
#include "Button.h"
#include "LEDC.h"
#include "Pin.h"

using APA102::LED;

class ClockArtTask: public ArtTask {
private:
    struct Mode {
    private:
	static char const * const string[];
    public:
	enum Value {clock, slide, spin} value;
	Mode(Value);
	Mode(char const *);
	char const * toString() const;
    };
    Mode				mode;
    KeyValueBroker::Observer const	modeObserver;

    uint64_t microsecondsSinceBootOfLastPeriod;

    unsigned updated;
    void update_();
    void update();

protected:
    void run() override;

public:
    ClockArtTask(
	SPI::Bus const		(&spiBus)[2],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    ~ClockArtTask() override;
};
