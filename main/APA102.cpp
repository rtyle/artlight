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

}
