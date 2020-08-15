#pragma once

#include <asio.hpp>

#include "GammaEncode.h"
#include "KeyValueBroker.h"

/// A LightPreferences is a base class for an implementation that
/// that supports preferences for presentation based on ambient lighting

class LightPreferences {
public:
    struct Dim {
    private:
	static char const * const string[];
    public:
	enum Value {automatic, manual} value;
	Dim(Value);
	Dim(char const *);
	char const * toString() const;
    };
    struct Range {
    private:
	static char const * const string[];
    public:
	enum Value {clip, normalize} value;
	Range(Value);
	Range(char const *);
	char const * toString() const;
    };

protected:
    Range				range;
    KeyValueBroker::Observer const	rangeObserver;
    Dim					dim;
    KeyValueBroker::Observer const	dimObserver;
    unsigned				dimLevel;
    KeyValueBroker::Observer const	dimLevelObserver;

    GammaEncode				gammaEncode;
    unsigned				gamma;
    KeyValueBroker::Observer const	gammaObserver;

    LightPreferences(
	asio::io_context &	io,
	KeyValueBroker &	keyValueBroker);
};
