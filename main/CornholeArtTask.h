#pragma once

#include "DialArtTask.h"
#include "Button.h"
#include "I2C.h"
#include "LEDC.h"
#include "Pin.h"
#include "SensorTask.h"
#include "SPI.h"
#include "TSL2561LuxSensor.h"

using APA102::LED;

class CornholeArtTask: public DialArtTask {
private:
    SPI::Bus const spiBus;
    SPI::Device const spiDevice;

    I2C::Master const i2cMaster;

    SensorTask sensorTask;
    TSL2561LuxSensor luxSensor;

    ObservablePin::ISR	pinISR;
    ObservablePin::Task	pinTask;
    ObservablePin	pin[4];

    Button		button[4];

    LEDC::Timer		ledTimerHighSpeed;
    LEDC::Timer		ledTimerLowSpeed;
    LEDC::Channel	ledChannel[3][3];

    unsigned				score[2];
    KeyValueBroker::Observer const	scoreObserver[2];

    struct Mode {
    private:
	static char const * const string[];
    public:
	enum Value {score, clock, slide, spin} value;
	Mode(Value);
	Mode(char const *);
	char const * toString() const;
    };
    Mode				mode;
    KeyValueBroker::Observer const	modeObserver;

    uint64_t microsecondsSinceBootOfBoardEvent;
    uint64_t microsecondsSinceBootOfHoleEvent;
    uint64_t microsecondsSinceBootOfLastPeriod;

    void boardEvent();
    void holeEvent();

    void scoreIncrement(size_t index, unsigned count);
    void scoreDecrement(size_t index, int count);
    void scoreObserved (size_t index, char const * value);

    unsigned updated;
    void update_();
    void update();

protected:
    void run() override;

public:
    CornholeArtTask(
	KeyValueBroker &	keyValueBroker);

    void start() override;

    ~CornholeArtTask() override;
};
