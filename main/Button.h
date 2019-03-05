#pragma once

#include "Pin.h"
#include "Timer.h"

/// A Button is an ObservablePin::Observer that accumulates its observations
/// into button press and hold events while debouncing the pin.
/// Multiple button presses (separated by a joinDuration or less)
/// are joined together to make one press(count) event
/// (0 => single press, 1 => double press, ...).
/// While the button is held down, every holdDuration a hold(count) event is
/// made (0 => first time).
class Button : private ObservablePin::Observer {
private:
    enum State {
	idle,
	debounce,
	down,
	up,
    };
    int const			downLevel;
    unsigned const		debounceDuration;
    unsigned const		joinDuration;
    unsigned const		holdDuration;
    std::function<void(unsigned)> const	press;
    std::function<void(unsigned)> const	hold;
    int				downCount;
    int				upCount;
    unsigned			holdCount;
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
	unsigned			debounceDuration,
	unsigned			joinDuration,
	unsigned			holdDuration,
	std::function<void(unsigned)>	press,
	std::function<void(unsigned)>	hold);

    /// Define an explicit move constructor
    /// because one will not be defined implicitly
    /// due to our non-copiable/non-movable std::recursive_mutex.
    /// A move constructor is needed to support initialization of an
    /// Button array class member.
    Button(Button const && move);
};
