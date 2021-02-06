#include "APA102.h"

namespace APA102 {

uint8_t nibble(char c) {
    if (std::isdigit(c)) {
        return c - '0';
    } else if (std::isxdigit(c)) {
        return 10 + (std::tolower(c) - 'a');
    } else {
        return 0;
    }
}

bool isColor(char const * c) {
    if (*c++ != '#') return false;
    for (auto i = 6; i--; c++) {
	if (!(std::isdigit(*c) || std::isxdigit(*c))) return false;
    }
    if (*c) return false;
    return true;
}

// construct LED<uint8_t> directly from encoding
template <> LED<uint8_t>::LED(uint32_t encoding_)
    : encoding(encoding_) {}

// return encoding directly from LED<uint8_t>
template <> LED<uint8_t>::operator uint32_t () const
    {return encoding;}

// construct other LED<T> variants indirectly
template <typename T> LED<T>::LED(uint32_t encoding)
    : LED(LED<uint8_t>(encoding)) {}

// variant instantiations
template LED<int>::LED(uint32_t);
template LED<unsigned>::LED(uint32_t);

// return encoding indirectly from other LED<T> variants
template <typename T> LED<T>::operator uint32_t () const
    {return LED<uint8_t>(*this);}

// variant instantiations
template LED<int>::operator uint32_t() const;
template LED<unsigned>::operator uint32_t() const;

// for encodings from LED<T> where T has more precision than uint8_t,
// we could get more precision using the low order 5 bits in part.control
// as it scales all the values down (using PWM) by this number over 31.
// implement LED<int16_t> this way.
// whites will be clipped to 0xfff and blacks will be clipped to 0.
// values will effectively be shifted right by 4 by scaling as much as possible
// first before shifting (which is where precision is lost).
template <> LED<int16_t>::operator uint32_t () const {
    constexpr int16_t	maxIn 		{0x0fff};
    constexpr uint8_t	maxOut		{0xff};
    constexpr unsigned	maxInClz	{__builtin_clz(maxIn)};
    constexpr unsigned	maxOutClz	{__builtin_clz(maxOut)};
    constexpr unsigned	shiftTotal	{maxOutClz - maxInClz};
    int16_t color[3] {
	*(&part.control + 3),
	*(&part.control + 2),
	*(&part.control + 1),
    };
    unsigned shiftPart {0};
    for (auto & e: color) { if (e) {
	if (e < 0) e = 0;		// clip blacks
	else {
	    unsigned shiftPart_ {0};
	    if (e > maxIn) {
		e = maxIn;		// clip whites
		shiftPart_ = shiftTotal;
	    } else {
		shiftPart_ = maxOutClz - std::min(
		    maxOutClz, static_cast<unsigned>(__builtin_clz(e)));
	    }
	    shiftPart = std::max(shiftPart, shiftPart_);
	}
    }}
    uint32_t encoding {0};
    for (auto & e: color) {
	encoding |= (e >> shiftPart);
	encoding <<= 8;
    }
    return encoding | 0b11100000 | (0b11111 >> (shiftTotal - shiftPart));
}

}
