#pragma once

#include <asio.hpp>

#include "APA102.h"
#include "KeyValueBroker.h"

/// A DialPreferences is a base class for an implementation that
/// that supports preferences for LED dial indicators

class DialPreferences {
private:
    asio::io_context & io_;

public:
    struct Shape {
    private:
	static char const * const string[];
    public:
	enum Value {bell, wave, bloom} value;
	Shape(Value);
	Shape(char const *);
	char const * toString() const;
    };

    void widthObserved(size_t index, char const * value);
    void colorObserved(size_t index, char const * value);
    void shapeObserved(size_t index, char const * value);

protected:
    float				width[3];
    APA102::LED<>			color[3];
    Shape				shape[3];

    KeyValueBroker::Observer const	widthObserver[3];
    KeyValueBroker::Observer const	colorObserver[3];
    KeyValueBroker::Observer const	shapeObserver[3];

    DialPreferences(
	asio::io_context &	io,
	KeyValueBroker &	keyValueBroker);
};
