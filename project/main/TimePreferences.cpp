#include <cstdlib>
extern "C" int setenv(char const *, char const *, int);

#include "TimePreferences.h"

TimePreferences::TimePreferences(
    asio::io_context &		io,
    KeyValueBroker &		keyValueBroker,
    size_t			smoothTimeStepCount)
:
    // timezone affects our notion of the localtime we present
    // forward a copy of any update to our task to make a synchronous change
    timezoneObserver(keyValueBroker, "timezone", CONFIG_TIME_ZONE,
	    [this, &io](char const * timezone){
		std::string timezoneCopy(timezone);
		io.post([timezoneCopy](){
		    setenv("TZ", timezoneCopy.c_str(), true);
		});
	    }),

    smoothTime	("smoothTime", smoothTimeStepCount)
{}
