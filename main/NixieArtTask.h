#pragma once

#include <array>

#include "APA102.h"
#include "ArtTask.h"
#include "I2C.h"
#include "PCA9685.h"
#include "SensorTask.h"
#include "SPI.h"
#include "TSL2561LuxSensor.h"
#include "TSL2591LuxSensor.h"
#include "HT7M2xxxMotionSensor.h"

class NixieArtTask: public ArtTask {
private:
    SPI::Bus const spiBus;
    SPI::Device const spiDevice;

    std::array<I2C::Master, 2> const i2cMasters;

    std::array<PCA9685, 4> pca9685s;

    SensorTask sensorTask;
//    TSL2591LuxSensor tsl2591LuxSensor;
    //HT7M2xxxMotionSensor motionSensor;

    void levelObserved(size_t index, char const * value);
    void colorObserved(size_t index, char const * value);

protected:
    float				level[2];
    APA102::LED<>			color[2];

    KeyValueBroker::Observer const	levelObserver[2];
    KeyValueBroker::Observer const	colorObserver[2];

    uint64_t microsecondsSinceBootOfModeChange;

    struct Mode {
    private:
	static char const * const string[];
    public:
	enum Value {clock, count, roll, clean} value;
	Mode(Value);
	Mode(char const *);
	char const * toString() const;
    };
    Mode				mode;
    KeyValueBroker::Observer const	modeObserver;

    void run() override;

private:
    unsigned updated;
    void update_();
    void update();

public:
    NixieArtTask(
	KeyValueBroker &	keyValueBroker);

    ~NixieArtTask() override;
};
