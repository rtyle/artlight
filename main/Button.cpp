#include "Button.h"

Button::Button(
    ObservablePin &			observablePin,
    int					downLevel_,
    unsigned				debounceDuration_,
    unsigned				bufferDuration_,
    unsigned				holdDuration_,
    std::function<void(unsigned)>	pressed_,
    std::function<void(int)>		held_)
:
    ObservablePin::Observer(observablePin, [this](){update(false);}),
    downLevel		(downLevel_),
    debounceDuration	(debounceDuration_),
    bufferDuration	(bufferDuration_),
    holdDuration	(holdDuration_),
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
    debounceDuration	(move.debounceDuration),
    bufferDuration	(move.bufferDuration),
    holdDuration	(move.holdDuration),
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
	if (timeout && debounceDuration < esp_timer_get_time() - stateTime) {
	    stateTime = esp_timer_get_time();
	    if (downLevel == observablePin.get_level()) {
		upCount = downCount++;
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
	    if (bufferDuration < stateDuration) {
		if (0 < upCount) {
		    pressed(upCount - 1);
		    upCount = downCount = 0;
		}
	    }
	    if (holdDuration < stateDuration) {
		stateTime += holdDuration;
		held(heldCount++);
	    }
	} else {
	    if (heldCount) {
		held(-heldCount);
		heldCount =  0;
		downCount =  0;
		upCount	  = -1;
	    }
	    stateTime = esp_timer_get_time();
	    state = bounce;
	}
	break;
    case up:
	if (timeout) {
	    int64_t stateDuration = esp_timer_get_time() - stateTime;
	    if (bufferDuration < stateDuration) {
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

bool Button::isDown() {return State::down == state;}
