#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "GammaEncode.h"

namespace APA102 {

// decode a hex nibble if we can from c (0-15); otherwise, 0
uint8_t nibble(char c);

// APA102C protocol used by adafruit dotstar devices

// https://cdn-shop.adafruit.com/datasheets/APA102.pdf
//
// https://en.wikipedia.org/wiki/Serial_Peripheral_Interface
//
// https://cpldcpu.wordpress.com/2014/08/27/apa102/
// https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
// https://cpldcpu.files.wordpress.com/2014/08/apa-102c-super-led-specifications-2014-en.pdf
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
// the clock needs to cycle N * 65 / 2 times.
// this is done by padding the data after the last LED.
// the padded content does not matter much;
// it just causes the clock to run longer.
// padding with ones is best as 32 zeros would start a new message.
// too little padding will result in data not reaching trailing LEDs.
// too much padding will be wasted off the end.

size_t constexpr messageBits(size_t size) {return size * 65 / 2;}

template<size_t size>
struct Message {
private:
    uint32_t	start;
public:
    uint32_t	encodings[size];
private:
    uint8_t	pad[(messageBits(size) + 7 / 8) - sizeof encodings - sizeof start];
public:
    Message() : start(0) {std::memset(pad, ~0, sizeof pad);}
    size_t length() const {return messageBits(size);}
    void gamma();
};

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
	T control, blue, green, red;
	Part(T red_, T green_, T blue_) :
	    control(~0), blue(blue_), green(green_), red(red_) {}
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

    template <typename F> LED operator * (F multiplier) const {
	return LED(
	    part.red	* multiplier,
	    part.green	* multiplier,
	    part.blue	* multiplier);
    }

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

    operator std::string () const;
};

}
