#include <iomanip>
#include <sstream>

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

template <> LED<uint8_t>::operator std::string () const {
    std::ostringstream stream;
    stream << '#' << std::hex << std::setw(2) << std::setfill('0')
	<< static_cast<unsigned>(part.red)
	<< static_cast<unsigned>(part.green)
	<< static_cast<unsigned>(part.blue);
    return stream.str();
}

// return gamma corrected encoding indirectly from other LED<T> variants
template <typename T> uint32_t LED<T>::gamma() const
    {return LED<uint8_t>(*this).gamma();}

// variant instantiations
template uint32_t LED<int>::gamma() const;
template uint32_t LED<unsigned>::gamma() const;

template <> uint32_t LED<uint8_t>::gamma() const {
    // 8-bit (2.6) gamma correction table from Adafruit_Dotstar.cpp
    static uint8_t const table[256] = {
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
	  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
	  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
	  7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
	 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
	 20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
	 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
	 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
	 58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
	 76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
	 97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
	122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
	150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
	182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
	218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255};
    return LED<uint8_t>(table[part.red], table[part.green], table[part.blue]);
}

}
