#pragma once

#include "Pin.h"
#include "Timer.h"

/// A Button is an ObservablePin::Observer that, together with a timer,
/// manages a finite state machine to debounce the pin and generate
/// pressed and held events.
/// An idle button enters the bounce state on any pin activity and stays
/// there until bounceDuration elapses.
/// After bounceDuration, the button enters the up or down state depending
/// on how the pin level compares with downLevel.
/// If the button is up or down for more than flushDuration,
/// any previously buffered pressed events are flushed.
/// While the button is down, each heldDuration a held event is generated.
/// If there is pin activity on an up button before a pressed event
/// generated, that event is buffered pending a flush and the bounce state
/// is entered.
class Button : private ObservablePin::Observer {
private:
    enum State {
	idle,
	bounce,
	down,
	up,
    };
    int const			downLevel;
    unsigned const		bounceDuration;
    unsigned const		flushDuration;
    unsigned const		heldDuration;
    std::function<void(unsigned)> const	pressed;
    std::function<void(unsigned)> const	held;
    int				downCount;
    int				upCount;
    unsigned			heldCount;
    std::recursive_mutex	mutex;
    Timer			timer;
    int64_t			stateTime;
    State			state;
    void update(bool timeout);
public:
    /// Construct a Debounce ObservablePin::Observer.
    /// Durations are expressed in microseconds.
    /// These are thresholds that are examined on tick granularity.
    Button(
	ObservablePin &			observablePin,
	int				downLevel,
	unsigned			bounceDuration,
	unsigned			flushDuration,
	unsigned			heldDuration,
	std::function<void(unsigned)>	pressed,
	std::function<void(unsigned)>	held);

    /// Define an explicit move constructor
    /// because one will not be defined implicitly
    /// due to our non-copiable/non-movable std::recursive_mutex.
    /// A move constructor is needed to support initialization of a
    /// Button class array class element.
    Button(Button const && move);
};
