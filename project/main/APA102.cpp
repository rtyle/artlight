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

// for encodings from LED<T> where T has more resolution than uint8_t,
// we could get more precision using the low order 5 bits in part.control
// as it scales all the values down (using PWM) by this number over 31.
// implement LED<int16_t> this way.
// whites in will be clipped to 0xfff and blacks will be clipped to 0.
// values out will effectively be shifted right by 4
// by scaling as much as possible first
// before shifting (which is where precision is lost).
template <> LED<int16_t>::operator uint32_t () const {
    constexpr int16_t	inMax 		{0x0fff};
    constexpr uint8_t	outMax		{0xff};
    constexpr unsigned	inMaxClz	{__builtin_clz(inMax)};
    constexpr unsigned	outMaxClz	{__builtin_clz(outMax)};
    constexpr unsigned	shiftTotal	{outMaxClz - inMaxClz};
    int16_t ins[3] {
	*(&part.control + 3),
	*(&part.control + 2),
	*(&part.control + 1),
    };
    unsigned shiftEach {0};
    for (auto & in: ins) { if (in) {
	if (in < 0) in = 0;		// clip blacks
	else {
	    unsigned shift {0};
	    if (in > inMax) {
		in = inMax;		// clip whites
		shift = shiftTotal;
	    } else {
		shift = outMaxClz - std::min(
		    outMaxClz, static_cast<unsigned>(__builtin_clz(in)));
	    }
	    if (shiftEach < shift) shiftEach = shift;
	}
    }}
    uint32_t outs {0};
    for (auto & in: ins) {
	if (in) {
	    // we got something in.
	    uint32_t out {static_cast<uint32_t>(in >> shiftEach)};
	    if (!out) out = 1;	// make sure something goes out
	    outs |= out;
	}
	outs <<= 8;
    }
    return outs | 0b11100000 | (0b11111 >> (shiftTotal - shiftEach));
}

}
