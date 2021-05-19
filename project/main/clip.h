#pragma once

#include "APA102.h"

// implicit unsigned integral conversions
// widen with zero extension and narrow by truncation
// this explicit narrowing conversion
// clips to max if larger and min if smaller.
template <typename T, typename U>
U clip(T t) {
    U max = std::numeric_limits<U>::max();
    if (t > static_cast<T>(max)) return max;
    U min = std::numeric_limits<U>::min();
    if (t < static_cast<T>(min)) return min;
    return t;
}

template <typename T, typename U = uint8_t>
APA102::LED<U> clip(APA102::LED<T> t) {
    U max = std::numeric_limits<U>::max();
    U min = std::numeric_limits<U>::min();
    APA102::LED<T> maxT(max, max, max);
    APA102::LED<T> minT(min, min, min);
    return APA102::LED<U>(t.minByPart(maxT).maxByPart(minT));
}
