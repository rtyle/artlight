#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "GammaEncode.h"

namespace APA102 {

/// decode a hex nibble if we can from c (0-15); otherwise, 0
uint8_t nibble(char c);

/// Return true if color can be used to construct an LED
bool isColor(char const * color);

// APA102C protocol used by adafruit dotstar devices

// https://cdn-shop.adafruit.com/datasheets/APA102.pdf
//
// https://en.wikipedia.org/wiki/Serial_Peripheral_Interface
//
// https://cpldcpu.wordpress.com/2014/08/27/apa102/
// https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
// https://cpldcpu.files.wordpress.com/2014/08/apa-102c-super-led-specifications-2014-en.pdf
// https://cpldcpu.wordpress.com/2016/12/13/sk9822-a-clone-of-the-apa102/
//
// each message is composed of 32-bit words.
// the message starts after at least 32 zero bits (a sync "word").
// the message data has a word for each LED (see LED, below)
// in daisy chain order.
// each of these words starts with 3 bits of ones,
// to set them apart from the sync zeros.
//
// each LED inverts its input clock to its output.
// as data is sampled on upward clock transitions
// and these have been inverted,
// data output must be (is) delayed by 1/2 clock period.
// data consumed by an upstream LED is converted to zeros downstream.
// these zeros will be ignored downstream
// as the these LEDs are looking for the three starting one bits.
//
// in order to accommodate the delay after each LED,
// the clock needs to cycle once more for every two LEDs.
// for a message with one start word and N LED words.
// the clock needs to cycle 32 + N * 65 / 2 times.
// this is done by padding the data after the last LED.
// too little padding will result in data not reaching trailing LEDs.
// too much padding will be wasted off the end.
//
// the SK9822 (APA102 clone) updates the PWM registers in the first cycle
// after the next sync word, while
// the APA102 updates the PWM register immediately after receiving the data.
// if we pad with zeros and add a sync word at the end,
// then each LED in an SK9822 string will update after it first sees
// sync word clocked through it.

// APA102 expects SPI clock to idle high (CPOL 1)
// and data samples to be taken on the rising edge (CPHA 1)
auto constexpr spiCPOL = 1;
auto constexpr spiCPHA = 1;
// these are encoded in the SPI mode
auto constexpr spiMode = (spiCPOL << 1) | (spiCPHA << 0);

// an LED<T> encoding orders control, red, green and blue values
// of type T appropriately.
// while the encoding of LED<uint8_t> is the only one appropriate for the
// protocol, LED<int> is better for arithmetic manipulations
// as it supports negative numbers and is less likely to overflow.
//
// while this can be freely converted to and from uint32_t (4 byte) values,
// content should only be made by methods of this class to ensure correctness.
//
// a control byte starts the encoding with three ones in the high order bits.
// the remaining 5 bits in this byte are interpreted as the numerator of a
// fraction with an implied denominator of 31 (0b11111).
// this fraction is used to scale the red, green and blue values.
// the hardware uses high frequency Pulse Width Modulation (PWM)
// to realize red, green and blue values and further scales these
// with a lower frequency PWM using the scaling factor.
// as this lower frequency PWM may be noticed as flicker
// (and is not particularly useful), we force the scaling factor to 1 (0b11111).
// thus, the brightness must be encoded fully in the red, green and blue values.
template <typename T = uint8_t> union LED {
private:
    uint32_t encoding; // union aggregate meaningful only for LED<uint8_t>

public:
    struct Part {
	T control,
#ifdef APA102_RBG
	red, blue, green
#else
	blue, green, red
#endif
	;
	Part(T red_, T green_, T blue_) :
	    control(~0),
#ifdef APA102_RBG
	    red(red_), blue(blue_), green(green_)
#else
	    blue(blue_), green(green_), red(red_)
#endif
	{}
    } part;

    LED(T red, T green, T blue) : part(red, green, blue) {}

    LED() : LED(0, 0, 0) {}

    template <typename t> LED(LED<t> const & that) :
	LED(that.part.red, that.part.green, that.part.blue) {}

    template <typename t> LED(GammaEncode const & g, LED<t> const & that) :
	LED(g(that.part.red), g(that.part.green), g(that.part.blue)) {}

    LED(uint32_t encoding);

    LED(char const * c) : LED(
	nibble(c[1]) << 4 | nibble(c[2]),
	nibble(c[3]) << 4 | nibble(c[4]),
	nibble(c[5]) << 4 | nibble(c[6]))
    {}

    T min() const {return part.red < part.green
	    ? part.red < part.blue
		? part.red
		: part.blue
	    : part.green < part.blue
		? part.green
		: part.blue;
    }

    T max() const {return part.red > part.green
	    ? part.red > part.blue
		? part.red
		: part.blue
	    : part.green > part.blue
		? part.green
		: part.blue;
    }

    T sum() const {return part.red + part.green + part.blue;}

    T average() const {return sum() / 3;}

    template <typename t> bool operator < (LED<t> const & that) const {
	return average() < that.average();
    }

    template <typename t> bool operator > (LED<t> const & that) const {
	return that < *this;
    }

    template <typename F> LED operator * (F multiplier) const;

    template <typename F> LED operator / (F divisor) const {
	return LED(
	    part.red	/ divisor,
	    part.green	/ divisor,
	    part.blue	/ divisor);
    }

    template <typename t> LED operator + (LED<t> const & addend) const {
	return LED(
	    part.red	+ addend.part.red,
	    part.green	+ addend.part.green,
	    part.blue	+ addend.part.blue);
    }

    template <typename t> LED operator - (LED<t> const & subtrahend) const {
	return LED(
	    part.red	- subtrahend.part.red,
	    part.green	- subtrahend.part.green,
	    part.blue	- subtrahend.part.blue);
    }

    template <typename t> LED operator ^ (LED<t> const & operand) const {
	return LED(
	    part.red	^ operand.part.red,
	    part.green	^ operand.part.green,
	    part.blue	^ operand.part.blue);
    }

    template <typename t> LED minByPart(LED<t> const & that) const {
	return LED(
	    part.red	< that.part.red		? part.red	: that.part.red,
	    part.green	< that.part.green	? part.green	: that.part.green,
	    part.blue	< that.part.blue	? part.blue	: that.part.blue);
    }

    template <typename t> LED maxByPart(LED<t> const & that) const {
	return LED(
	    part.red	> that.part.red		? part.red	: that.part.red,
	    part.green	> that.part.green	? part.green	: that.part.green,
	    part.blue	> that.part.blue	? part.blue	: that.part.blue);
    }

    template <typename t> LED minByAverage(LED<t> const & that) const {
	return LED(*this < that ? *this : that);
    }

    template <typename t> LED maxByAverage(LED<t> const & that) const {
	return LED(*this > that ? *this : that);
    }

    template <typename t> LED average(LED<t> const & that) const {
	return LED(
	    (part.red	+ that.part.red)	/ 2,
	    (part.green	+ that.part.green)	/ 2,
	    (part.blue	+ that.part.blue)	/ 2);
    }

    operator uint32_t () const;

    // only makes sense for T = uint8_t
    operator std::string () const {
	std::ostringstream stream;
	stream << '#' << std::hex << std::setfill('0')
	    << std::setw(2) << static_cast<unsigned>(part.red)
	    << std::setw(2) << static_cast<unsigned>(part.green)
	    << std::setw(2) << static_cast<unsigned>(part.blue);
	return stream.str();
    }
};

template <typename T> template <typename F>
LED<T> LED<T>::operator * (F multiplier) const {
    return LED<T>(
	part.red	* multiplier,
	part.green	* multiplier,
	part.blue	* multiplier);
}

// int16_t specialization:
// do not allow non-zero parts to be scaled to zero unless they all are.
template <> template <typename F>
LED<int16_t> LED<int16_t>::operator * (F multiplier) const {
    if (0 < multiplier) {
	int16_t const ps[] {
	    part.red,
	    part.green,
	    part.blue,
	};
	int16_t min {std::numeric_limits<int16_t>::max()};
	int16_t max {0};
	for (auto & p: ps) {
	    int16_t const v {0 > p ? static_cast<int16_t>(-p) : p};
	    if (v) {
		if (min > v) min = v;
		if (max < v) max = v;
	    }
	}
	if (min < max
		&& 0 == static_cast<int16_t>(min * multiplier)
		&& 0 != static_cast<int16_t>(max * multiplier)) {
	    multiplier = static_cast<F>(1) / min;
	}
    }
    return LED<int16_t> {
	static_cast<int16_t>(part.red	* multiplier),
	static_cast<int16_t>(part.green	* multiplier),
	static_cast<int16_t>(part.blue	* multiplier)};
}

std::size_t constexpr messageBits(std::size_t size) {return 32 + size * 65 / 2 + 32;}

template<std::size_t size>
struct Message {
private:
    uint32_t	start;
public:
    uint32_t	encodings[size];
private:
    uint32_t	update;
    uint8_t	pad[(messageBits(size) + 7 / 8)
		    - sizeof start - sizeof encodings - sizeof update];
public:
    Message() :
	start		{},
	encodings	{},
	update		{},
	pad		{}
    {
	for (auto & e: encodings) {
	    e = LED<>();
	}
    }
    std::size_t length() const {return messageBits(size);}
    void gamma();
};

}
