#pragma once

#include "AsioTask.h"
#include "DialPreferences.h"
#include "I2C.h"
#include "KeyValueBroker.h"
#include "LuxSensor.h"
#include "Pin.h"
#include "SensorTask.h"
#include "SPI.h"
#include "TimePreferences.h"

using APA102::LED;

class GoldenArtTask: public AsioTask, TimePreferences, DialPreferences {
private:
    Pin			tinyPicoLedPower;
    SPI::Bus const	spiBus[2];
    SPI::Device const	spiDevice[2];
    I2C::Master const	i2cMaster;

    SensorTask			sensorTask;
    std::unique_ptr<LuxSensor>	luxSensor;

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
    unsigned				curl[dialCount];
    unsigned				length[dialCount];
    float				black;
    float				white;
    float				level;
    float				dim;
    float				gamma;

    KeyValueBroker::Observer const	modeObserver;
    KeyValueBroker::Observer const	curlObserver[dialCount];
    KeyValueBroker::Observer const	lengthObserver[dialCount];
    KeyValueBroker::Observer const	blackObserver;
    KeyValueBroker::Observer const	whiteObserver;
    KeyValueBroker::Observer const	levelObserver;
    KeyValueBroker::Observer const	dimObserver;
    KeyValueBroker::Observer const	gammaObserver;

    unsigned updated;

    void curlObserved(size_t index, char const * value);
    void lengthObserved(size_t index, char const * value);

    void update_();
    void update();

protected:
    void run() override;

public:
    GoldenArtTask(
	KeyValueBroker &	keyValueBroker);

    ~GoldenArtTask() override;
};
