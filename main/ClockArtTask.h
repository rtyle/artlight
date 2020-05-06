#pragma once

#include "DialArtTask.h"
#include "Button.h"
#include "I2C.h"
#include "LuxTask.h"
#include "LEDC.h"
#include "Pin.h"
#include "SPI.h"

using APA102::LED;

class ClockArtTask: public DialArtTask {
private:
    SPI::Bus const spiBus[2];
    SPI::Device const spiDevice[2];

    I2C::Master const i2cMaster;

    LuxTask luxTask;

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

    bool				reverse;
    KeyValueBroker::Observer const	reverseObserver;

    uint64_t microsecondsSinceBootOfLastPeriod;

    unsigned updated;
    void update_();
    void update();

protected:
    void run() override;

public:
    ClockArtTask(
	KeyValueBroker &	keyValueBroker);

    ~ClockArtTask() override;
};
