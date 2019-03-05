#include "Button.h"

Button::Button(
    ObservablePin &			observablePin,
    int					downLevel_,
    unsigned				debounceDuration_,
    unsigned				joinDuration_,
    unsigned				holdDuration_,
    std::function<void(unsigned)>	press_,
    std::function<void(unsigned)>	hold_)
:
    ObservablePin::Observer(observablePin, [this](){update(false);}),
    downLevel		(downLevel_),
    debounceDuration	(debounceDuration_),
    joinDuration	(joinDuration_),
    holdDuration	(holdDuration_),
    press		(press_),
    hold		(hold_),
    downCount		(0),
    upCount		(0),
    holdCount		(0),
    mutex		(),
    timer		("Button", 1, true, [this](){update(true);}),
    stateTime		(esp_timer_get_time()),
    state		(idle)
{}

Button::Button(Button const && move)
:
    ObservablePin::Observer(move),
    downLevel		(move.downLevel),
    debounceDuration	(move.debounceDuration),
    joinDuration	(move.joinDuration),
    holdDuration	(move.holdDuration),
    press		(std::move(move.press)),
    hold		(std::move(move.hold)),
    downCount		(move.downCount),
    upCount		(move.upCount),
    holdCount		(move.holdCount),
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
	    state = debounce;
	}
	break;
    case debounce:
	if (timeout && debounceDuration < esp_timer_get_time() - stateTime) {
	    stateTime = esp_timer_get_time();
	    if (downLevel == observablePin.get_level()) {
		upCount = downCount++;
		holdCount = 0;
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
	    if (joinDuration < stateDuration) {
		if (0 < upCount) {
		    press(upCount - 1);
		    upCount = downCount = 0;
		}
	    }
	    if (holdDuration < stateDuration) {
		stateTime += holdDuration;
		hold(holdCount++);
		upCount = downCount = -1;
	    }
	} else {
	    stateTime = esp_timer_get_time();
	    state = debounce;
	}
	break;
    case up:
	if (timeout) {
	    int64_t stateDuration = esp_timer_get_time() - stateTime;
	    if (joinDuration < stateDuration) {
		if (0 < upCount) {
		    press(upCount - 1);
		    upCount = downCount = 0;
		}
		timer.stop();
		stateTime = esp_timer_get_time();
		state = idle;
	    }
	} else {
	    stateTime = esp_timer_get_time();
	    state = debounce;
	}
	break;
    }
}
