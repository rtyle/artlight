#pragma once

#include "asio.hpp"

#include "KeyValueBroker.h"
#include "SmoothTime.h"

/// A TimePreferences is a base class for an implementation that
/// that supports preferences for time presentation.

class TimePreferences {
protected:

    KeyValueBroker::Observer const	timezoneObserver;
    SmoothTime				smoothTime;

    TimePreferences(
	asio::io_context &	io,
	KeyValueBroker &	keyValueBroker,
	size_t			smoothTimeStepCount);
};
