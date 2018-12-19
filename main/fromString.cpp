#include <sstream>
#include <string>

#include "fromString.h"

template <>
bool fromString(char const * s) {
    switch (*s) {
    case 0:
    case '0':
    case 'f':
    case 'F':
        return false;
    }
    return true;
}

template <typename T>
static T fromString(char const * s) {
    std::istringstream stream(s);
    T t;
    stream >> t;
    return t;
}

// variant instantiations
template float fromString(char const *);
