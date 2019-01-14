#include <cmath>

#include <esp_timer.h>

#include "SmoothTime.h"

static auto constexpr millisecondsPerSecond	= 1000u;
static auto constexpr millisecondsPerMinute	= 60u * millisecondsPerSecond;
static auto constexpr millisecondsPerHour	= 60u * millisecondsPerMinute;

static auto constexpr microsecondsPerMillisecond = 1000u;

// we want to get (and present) local time of day at a subsecond resolution.
// the resolution of std::time is a second.
// the resolution of clock_gettime(CLOCK_REALTIME,...) is a nanosecond.
// these functions are implemented in time.c of the newlib component of esp-idf.
// the resolution of time keeping on the esp32 platform is a microsecond.
//
// 1. because their is no battery backed timekeeper, time is lost when power is.
// 2. time will drift due to the inaccuracy of the platform's hardware timer.
// we expect both of these issues to be addressed with lwIP SNTP.
// lwIP SNTP does not use adjtime to slew small time drifts (2).
// rather, it always uses settimeofday to resolve big (1) and small time
// differences.
//
// the newlib implementation of settimeofday (and adjtime) just jumps (or slews)
// a private "boot time" variable in time.c.
// this variable is set so that when the platform's
// "microseconds since boot time" measure is added to it
// we get the current time.
//
// we would like our presentation of time to not immediately jump
// to the SNTP corrected time.
// rather, we would like to see it gradually snap into place.
// this way, hour, minute and second hands might animate as they would on an
// analog clock when someone spins the second hand to change the time.
//
// to do this, we need to know when these jumps occur and how big they are.
// different timers might be compared to do this but it would sacrifice
// some accuracy (time elapses between samples) and risk being scheduled out
// between samples.
// a simpler, more reliable implementation would be to rely on being able
// to access the private "boot time" variable in time.c
// so, that is what we have done (see our modified copy here).
//
// although time.c advertises this as uint64_t, at boot time it has been seen
// to wrap backwards from 0 about a second (a negative number).
// so, we will interpret it as such.
extern "C" int64_t get_boot_time();

SmoothTime::SmoothTime(char const * name_, size_t stepCount_)
:
    name(name_),
    stepCount(stepCount_),
    stepLeft(0),
    stepProduct(0),
    stepFactor(0),
    lastBootTime(get_boot_time())
{}

int64_t SmoothTime::microsecondsSinceEpoch() {
    uint64_t microseconds = esp_timer_get_time();
    int64_t const thisBootTime = get_boot_time();
    if (thisBootTime != lastBootTime) {
	// if we were to cut the difference in half on each successive step
	// there would be log2(abs(thisBootTime - lastBootTime)) steps needed.
	// the corrected boot time on step N would be
	//	thisBootTime - (thisBootTime - lastBootTime) * 1 / 2**N
	// this can be optimized if we accumulate a stepProduct on each step
	// so that the corrected boot time on the next step would be
	//	thisBootTime - (stepProduct = stepProduct * stepFactor)
	// stepProduct would start as thisBootTime - lastBootTime.
	// to cut the difference in half each time, stepFactor would be 1/2.
	// to stretch the number of steps to make the same transition,
	// we just need to affect the stepFactor (see below).
	stepProduct  = thisBootTime - lastBootTime;
	lastBootTime = thisBootTime;

	// since our result is used to animate a 12 hour clock
	// it makes no sense to step over a larger amount.
	// remove full turns to get within one turn centered on thisBootTime.
	static auto constexpr turn = 12 * 60 * 60 * 1000000ull;
	static auto constexpr halfTurn = turn / 2;
	if (0.0f < stepProduct) {
	    stepProduct -= (
		(static_cast<uint64_t>( stepProduct) + halfTurn) / turn) * turn;
	} else {
	    stepProduct += (
		(static_cast<uint64_t>(-stepProduct) + halfTurn) / turn) * turn;
	}

	if (stepProduct) {
	    stepLeft = stepCount;
	    float halfStepCount
		= std::log2(0.0f < stepProduct ? stepProduct : -stepProduct);
	    stepFactor = std::exp2(-1.0f * halfStepCount / stepCount);
//	    ESP_LOGI(name, "thisBootTime=%lld stepProduct=%f halfStepCount=%f stepCount=%zu stepFactor=%f",
//		thisBootTime, stepProduct, halfStepCount, stepCount, stepFactor);
	}
    }
    if (!stepLeft) {
//	ESP_LOGI(name, "%llu (now) = %lld (thisBootTime) + %llu (us)",
//	    thisBootTime + microseconds, thisBootTime, microseconds);
	return thisBootTime + microseconds;
    } else {
	--stepLeft;
	stepProduct *= stepFactor;
	int64_t now = thisBootTime - static_cast<int64_t>(stepProduct) + microseconds;
//	ESP_LOGI(name, "%lld (now) = %lld (thisBootTime) - %lld (stepProduct) + %llu (us)",
//	    now, thisBootTime, static_cast<int64_t>(stepProduct), microseconds);
	return now;
    }
}

uint32_t SmoothTime::millisecondsSinceTwelveLocaltime() {
    int64_t milliseconds
	= microsecondsSinceEpoch() / microsecondsPerMillisecond;

    std::time_t seconds = milliseconds / millisecondsPerSecond;
    milliseconds -= static_cast<int64_t>(seconds) * millisecondsPerSecond;

    std::tm tm;
    localtime_r(&seconds, &tm);

    return milliseconds
	+ (tm.tm_sec      ) * millisecondsPerSecond
	+ (tm.tm_min      ) * millisecondsPerMinute
	+ (tm.tm_hour % 12) * millisecondsPerHour;
}
