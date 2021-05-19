#pragma once

#include "AsioTask.h"
#include "Button.h"
#include "DialPreferences.h"
#include "I2C.h"
#include "KeyValueBroker.h"
#include "LEDC.h"
#include "LightPreferences.h"
#include "Pin.h"
#include "SensorTask.h"
#include "SPI.h"
#include "TSL2561LuxSensor.h"
#include "TimePreferences.h"

using APA102::LED;

class ClockArtTask: public AsioTask, TimePreferences, DialPreferences, LightPreferences {
private:
    SPI::Bus const spiBus[2];
    SPI::Device const spiDevice[2];

    I2C::Master const i2cMaster;

    SensorTask sensorTask;
    TSL2561LuxSensor luxSensor;

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
