#include <cmath>

#include "Dial.h"

static float constexpr pi	= std::acos(-1.0f);
static float constexpr tau	= 2.0f * pi;

Dial::Dial(float position_) : position(position_) {}

float Dial::operator()(float place) const {
    float offset = place - position;
    float ignore;
    if (offset < 0.0f) {
	offset = 1.0f - std::modf(-offset, &ignore);
    } else {
	offset = std::modf(offset, &ignore);
    }
    if (offset >= 0.5f) offset -= 1.0f;
    return offset;
}

BumpDial::BumpDial(float position, float width_)
    : Dial(position), width(2.0f * width_) {}

float BumpDial::operator()(float place) const {
    return std::abs(std::cos(tau * Dial::operator()(place) / width));
}

BellDial::BellDial(float position, float sigma)
    : Dial(position), twoSigmaSquared(2.0f * sigma * sigma) {}


float BellDial::operator()(float place) const {
    float offset = Dial::operator()(place);
    return std::exp(-offset * offset / twoSigmaSquared);
}

WaveDial::WaveDial(float position, float width_)
    : Dial(position), width(width_) {}


float WaveDial::operator()(float place) const {
    return (1.0f + std::cos(tau * Dial::operator()(place) / width)) / 2.0f;
}

BellStandingWaveDial::BellStandingWaveDial(
    float position,
    float sigma,
    float wavePosition,
    float waveWidth)
:
    BellDial(position, sigma),
    rightWaveDial(wavePosition, waveWidth),
    leftWaveDial(-wavePosition, waveWidth)
{}

float BellStandingWaveDial::operator()(float place) const {
    return BellDial::operator()(place)
	* (rightWaveDial(place) + leftWaveDial(place)) / 2.0f;
}

RippleDial::RippleDial(
    float position, float width_) : Dial(position), width(width_) {}

static float sinc(float x) {
    if (0.0f == x) return 1.0f;
    return std::sin(x) / x;
}

float RippleDial::operator()(float place) const {
    float offset = pi * std::abs(Dial::operator()(place) / width);
    if (1.0f / 2.0f > offset) {
	return sinc(offset);
    } else {
	return (1.0f + std::sin(offset)) / (2.0f * offset);
    }
}
