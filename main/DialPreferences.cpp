#include "DialPreferences.h"

#include "fromString.h"

char const * const DialPreferences::Shape::string[] {"bell", "wave", "bloom"};
DialPreferences::Shape::Shape(Value value_) : value(value_) {}
DialPreferences::Shape::Shape(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return bell;
    }()
) {}
char const * DialPreferences::Shape::toString() const {
    return string[value];
}

char const * const widthKey[] {
    "aWidth",
    "bWidth",
    "cWidth",
};
char const * const colorKey[] {
    "aColor",
    "bColor",
    "cColor",
};
char const * const shapeKey[] {
    "aShape",
    "bShape",
    "cShape",
};

void DialPreferences::widthObserved(size_t index, char const * value_) {
    float value = fromString<float>(value_);
    if (0.0f <= value && value <= 64.0f) {
	io_.post([this, index, value](){
	    width[index] = value;
	});
    }
}

void DialPreferences::colorObserved(size_t index, char const * value_) {
    if (APA102::isColor(value_)) {
	APA102::LED<> value(value_);
	io_.post([this, index, value](){
	    color[index] = value;
	});
    }
}

void DialPreferences::shapeObserved(size_t index, char const * value_) {
    Shape value(value_);
    io_.post([this, index, value](){
	shape[index] = value;
    });
}

DialPreferences::DialPreferences(
    asio::io_context &		io,
    KeyValueBroker &		keyValueBroker)
:
    io_ {io},

    width {},
    color {},
    shape {
	Shape::Value::bell,
	Shape::Value::bell,
	Shape::Value::bell,
    },

    widthObserver {
	{keyValueBroker, widthKey[0], "8",
	    [this](char const * value) {widthObserved(0, value);}},
	{keyValueBroker, widthKey[1], "8",
	    [this](char const * value) {widthObserved(1, value);}},
	{keyValueBroker, widthKey[2], "4",
	    [this](char const * value) {widthObserved(2, value);}},
    },

    colorObserver {
	{keyValueBroker, colorKey[0], "#ff0000",
	    [this](char const * value) {colorObserved(0, value);}},
	{keyValueBroker, colorKey[1], "#0000ff",
	    [this](char const * value) {colorObserved(1, value);}},
	{keyValueBroker, colorKey[2], "#ffff00",
	    [this](char const * value) {colorObserved(2, value);}},
    },

    shapeObserver {
	{keyValueBroker, shapeKey[0], shape[0].toString(),
	    [this](char const * value) {shapeObserved(0, value);}},
	{keyValueBroker, shapeKey[1], shape[1].toString(),
	    [this](char const * value) {shapeObserved(1, value);}},
	{keyValueBroker, shapeKey[2], shape[2].toString(),
	    [this](char const * value) {shapeObserved(2, value);}},
    }
{}