#include <esp_log.h>

#include "Timer.h"

// we cannot use xTimerCreateStatic with a StaticTimer_t Timer member here
// because the timer daemon/task might try to access this memory
// after its Timer has been destroyed.
// we still need to make sure that expireThat does not try to access a
// destroyed Timer object.
// we do this by setting the timer id to nullptr as part of destruction
// and check for this in expireThat.

void Timer::expireThis() {expire();}

/* static */ void Timer::expireThat(TimerHandle_t timerHandle) {
    // make sure we don't access a Timer that has been destroyed
    if (Timer * timer = static_cast<Timer *>(pvTimerGetTimerID(timerHandle))) {
	timer->expireThis();
    }
}

Timer::Timer(
    char const *		name_,
    TickType_t			period,
    UBaseType_t			reload,
    std::function<void()>	expire_)
:
    name(name_),
    expire(expire_),
    timerHandle(xTimerCreate(name, period, reload, this, expireThat))
{
    ESP_LOGI(name, "Timer::Timer period=%u", period);
}

void Timer::start() {
    ESP_LOGI(name, "Timer::start");
    xTimerStart(timerHandle, portMAX_DELAY);
}

void Timer::stop () {
    ESP_LOGI(name, "Timer::stop");
    xTimerStop (timerHandle, portMAX_DELAY);
}

Timer::~Timer() {
    ESP_LOGI(name, "Timer::~Timer delete");
    vTimerSetTimerID(timerHandle, nullptr);
    xTimerDelete(timerHandle, portMAX_DELAY);
}
