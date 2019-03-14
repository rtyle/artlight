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

    unsigned				aScore;
    KeyValueBroker::Observer const	aScoreObserver;

    unsigned				bScore;
    KeyValueBroker::Observer const	bScoreObserver;

    void update();

public:
    CornholeArtTask(
	SPI::Bus const		(&spiBus)[2],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    /* virtual */ void run() override;

    /* virtual */ ~CornholeArtTask() override;
};
