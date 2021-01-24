#pragma once

#include "AsioTask.h"
#include "DialPreferences.h"
#include "KeyValueBroker.h"
#include "Pin.h"
#include "SPI.h"
#include "TimePreferences.h"

using APA102::LED;

class GoldenArtTask: public AsioTask, TimePreferences, DialPreferences {
private:
    Pin			tinyPicoLedPower;
    SPI::Bus const	spiBus[2];
    SPI::Device const	spiDevice[2];

    struct Mode {
    private:
	static char const * const string[];
    public:
	enum Value {clock, swirl, solid} value;
	Mode(Value);
	Mode(char const *);
	char const * toString() const;
    };
    Mode				mode;
    KeyValueBroker::Observer const	modeObserver;

    unsigned updated;
    void update_();
    void update();

protected:
    void run() override;

public:
    GoldenArtTask(
	KeyValueBroker &	keyValueBroker);

    ~GoldenArtTask() override;
};
