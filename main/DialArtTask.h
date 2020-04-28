#pragma once

#include "APA102.h"

#include "ArtTask.h"

/// A DialArtTask is an abstract ArtTask
/// that supports preferences for LED dial indicators

class DialArtTask : public ArtTask {
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

    DialArtTask(
	char const *		name,
	UBaseType_t		priority,
	size_t			stackSize,
	BaseType_t		core,

	SPI::Bus const		(&spiBus)[2],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker,
	size_t			smoothTimeStepCount = 4096);
};
