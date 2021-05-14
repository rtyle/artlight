#pragma once

#include "Pin.h"
#include "Timer.h"

/// A Button is an ObservablePin::Observer that, together with a timer,
/// manages a finite state machine to debounce the pin and generate
/// pressed and held events.
/// An idle button enters the bounce state on any pin activity and stays
/// there until debounceDuration elapses.
/// After debounceDuration, the button enters the up or down state depending
/// on how the pin level compares with downLevel.
/// If there is pin activity on an up button before a pressed event
/// is generated, that event is buffered and the bounce state is reentered;
/// otherwise, if it has been up for more than bufferDuration any buffered
/// events are flushed (pressed is called with the buffered count)
/// and the idle state is entered.
/// If the button is down for more than bufferDuration, any buffered
/// events are flushed (pressed is called with the buffered count).
/// While the button is down, each holdDuration, held is called with
/// incrementing counter.
/// When a held button is finally released, held is called with the negated
/// counter.
class Button : private ObservablePin::Observer {
private:
    enum State {
	idle,
	bounce,
	down,
	up,
    };
    int const			downLevel;
    unsigned const		debounceDuration;
    unsigned const		bufferDuration;
    unsigned const		holdDuration;
    std::function<void(unsigned)> const	pressed;
    std::function<void(unsigned)> const	held;
    unsigned			downCount;
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
	unsigned			debounceDuration,
	unsigned			bufferDuration,
	unsigned			holdDuration,
	std::function<void(unsigned)>	pressed,
	std::function<void(int)>	held);

    /// Define an explicit move constructor
    /// because one will not be defined implicitly
    /// due to our non-copiable/non-movable std::recursive_mutex.
    /// A move constructor is needed to support initialization of a
    /// Button class array class element.
    Button(Button const && move);

    bool isDown() const;
};
