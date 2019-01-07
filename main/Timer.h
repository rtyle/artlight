#pragma once

#include <functional>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

/// A Timer wraps a FreeRTOS Timer, that, once started, calls its expire
/// function object on expiration.
class Timer {
private:
    char const *		name;
    std::function<void()>	expire;
    TimerHandle_t		timerHandle;

    void expireThis();
    static void expireThat(TimerHandle_t timerHandle);

public:
    Timer(
	char const *		name,
	TickType_t		period,
	UBaseType_t		reload,
	std::function<void()>	expire);

    void start();
    void stop ();

    void setPeriod(TickType_t);
    TickType_t getPeriod();

    ~Timer();
};
