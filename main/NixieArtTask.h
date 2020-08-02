#pragma once

#include <array>

#include "APA102.h"
#include "ArtTask.h"
#include "I2C.h"
#include "PCA9685.h"
#include "SensorTask.h"
#include "SPI.h"
#include "LuxSensor.h"
#include "MotionSensor.h"

class NixieArtTask: public ArtTask {
public:
    static size_t constexpr sideCount	{2};
    static size_t constexpr placeCount	{4};
    static size_t constexpr ledCount	{sideCount * placeCount};

private:
    SPI::Bus const spiBus;
    SPI::Device const spiDevice;

    std::array<I2C::Master, 2> const i2cMasters;

    std::array<PCA9685, placeCount> pca9685s;

    SensorTask sensorTask;
    std::unique_ptr<LuxSensor>		luxSensor;
    std::unique_ptr<MotionSensor>	motionSensor;

    void levelObserved	(size_t index, char const * value);
    void dimObserved	(size_t index, char const * value);
    void colorObserved	(size_t index, char const * value);

protected:
    float				levels	[sideCount + 1];
    float				dims	[sideCount + 1];
    APA102::LED<>			colors	[sideCount];

    KeyValueBroker::Observer const	levelsObserver	[sideCount + 1];
    KeyValueBroker::Observer const	dimsObserver	[sideCount + 1];
    KeyValueBroker::Observer const	colorsObserver	[sideCount];

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
