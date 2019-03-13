#include <cmath>

#include "Sawtooth.h"

Sawtooth::Sawtooth(float period_) : period(period_) {}

float Sawtooth::operator()(float x) const {
    return std::modf(x / period, &x);
}
