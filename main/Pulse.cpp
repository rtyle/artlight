#include <cmath>

#include "Pulse.h"

static float constexpr pi	= std::acos(-1.0f);
static float constexpr tau	= 2.0f * pi;

Pulse::Pulse(float count, bool pulseFirst) :
    factor(count * tau),
    sign(pulseFirst ? 1 : -1)
{}

float Pulse::operator () (float x) const {
    // factor in sin(x * factor) will adjust the frequency as needed.
    // dividing the result by the same factor will adjust the magnitude
    // such that the magnitude of the slope will never be less than -1.
    // when adding this to a curve with a slope of 1 everywhere
    // (the identity, x) this ensures the result monotonically increases.
    return factor ? x + sign * std::sin(x * factor) / factor : x;
};
