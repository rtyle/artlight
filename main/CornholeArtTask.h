#pragma once

#include "ArtTask.h"
#include "Button.h"
#include "LEDC.h"
#include "Pin.h"

using APA102::LED;

class CornholeArtTask: public ArtTask {
private:
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

    void update();

protected:
    void run() override;

public:
    CornholeArtTask(
	SPI::Bus const		(&spiBus)[2],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    void start() override;

    ~CornholeArtTask() override;
};
