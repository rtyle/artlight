#include "Button.h"

Button::Button(
    ObservablePin &			observablePin,
    int					downLevel_,
    unsigned				bounceDuration_,
    unsigned				flushDuration_,
    unsigned				holdDuration_,
    std::function<void(unsigned)>	pressed_,
    std::function<void(unsigned)>	held_)
:
    ObservablePin::Observer(observablePin, [this](){update(false);}),
    downLevel		(downLevel_),
    bounceDuration	(bounceDuration_),
    flushDuration	(flushDuration_),
    heldDuration	(holdDuration_),
    pressed		(pressed_),
    held		(held_),
    downCount		(0),
    upCount		(0),
    heldCount		(0),
    mutex		(),
    timer		("Button", 1, true, [this](){update(true);}),
    stateTime		(esp_timer_get_time()),
    state		(idle)
{}

Button::Button(Button const && move)
:
    ObservablePin::Observer(move),
    downLevel		(move.downLevel),
    bounceDuration	(move.bounceDuration),
    flushDuration	(move.flushDuration),
    heldDuration	(move.heldDuration),
    pressed		(std::move(move.pressed)),
    held		(std::move(move.held)),
    downCount		(move.downCount),
    upCount		(move.upCount),
    heldCount		(move.heldCount),
    mutex		(),
    timer		(std::move(move.timer)),
    stateTime		(move.stateTime),
    state		(move.state)
{}

// update our state machine based on what state we are in and
// why we are being called (timeout or event).
void Button::update(bool timeout) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    switch (state) {
    case idle:
	if (!timeout) {	// ignore timer (leak)
	    timer.start();
	    stateTime = esp_timer_get_time();
	    state = bounce;
	}
	break;
    case bounce:
	if (timeout && bounceDuration < esp_timer_get_time() - stateTime) {
	    stateTime = esp_timer_get_time();
	    if (downLevel == observablePin.get_level()) {
		upCount = downCount++;
		heldCount = 0;
		state = down;
	    } else {
		downCount = ++upCount;
		state = up;
	    }
	}
	break;
    case down:
	if (timeout) {
	    int64_t stateDuration = esp_timer_get_time() - stateTime;
	    if (flushDuration < stateDuration) {
		if (0 < upCount) {
		    pressed(upCount - 1);
		    upCount = downCount = 0;
		}
	    }
	    if (heldDuration < stateDuration) {
		stateTime += heldDuration;
		held(heldCount++);
		upCount = downCount = -1;
	    }
	} else {
	    stateTime = esp_timer_get_time();
	    state = bounce;
	}
	break;
    case up:
	if (timeout) {
	    int64_t stateDuration = esp_timer_get_time() - stateTime;
	    if (flushDuration < stateDuration) {
		if (0 < upCount) {
		    pressed(upCount - 1);
		    upCount = downCount = 0;
		}
		timer.stop();
		stateTime = esp_timer_get_time();
		state = idle;
	    }
	} else {
	    stateTime = esp_timer_get_time();
	    state = bounce;
	}
	break;
    }
}
