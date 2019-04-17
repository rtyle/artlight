#include <cmath>

#include "Curve.h"

static float constexpr pi	= std::acos(-1.0f);
static float constexpr tau	= 2.0f * pi;

Curve::Curve(float position_) : position(position_) {}

Curve::~Curve() {}

float Curve::operator()(float place) const {
    return place - position;
}

Dial::Dial(float position) : Curve(position) {}

float Dial::operator()(float place) const {
    float offset = Curve::operator()(place);
    float ignore;
    if (offset < 0.0f) {
	offset = 1.0f - std::modf(-offset, &ignore);
    } else {
	offset = std::modf(offset, &ignore);
    }
    if (offset >= 0.5f) offset -= 1.0f;
    return offset;
}

BumpCurve::BumpCurve(float position, float width_)
    : Curve(position), width(2.0f * width_) {}

float BumpCurve::operator()(float place) const {
    return std::abs(std::cos(tau * Curve::operator()(place) / width));
}

template <typename T>
BellCurve<T>::BellCurve(float position, float width /* == 2 * sigma */)
    : T(position), twoSigmaSquared(width * width / 2.0f) {}
template BellCurve<>::BellCurve(float, float);
template BellCurve<Dial>::BellCurve(float, float);

template <typename T>
float BellCurve<T>::operator()(float place) const {
    float offset = T::operator()(place);
    return std::exp(-offset * offset / twoSigmaSquared);
}
template float BellCurve<>::operator()(float) const;
template float BellCurve<Dial>::operator()(float) const;

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
    BellCurve<Dial>(position, sigma),
    rightWaveDial(wavePosition, waveWidth),
    leftWaveDial(-wavePosition, waveWidth)
{}

float BellStandingWaveDial::operator()(float place) const {
    return BellCurve<Dial>::operator()(place)
	* (rightWaveDial(place) + leftWaveDial(place)) / 2.0f;
}

BloomDial::BloomDial(float position, float width_, float phase_)
    : Dial(position), width(pi * width_ / 4.0f), phase(tau * phase_) {}


float BloomDial::operator()(float place) const {
    float offset = std::abs(Dial::operator()(place));
    return 0.0f == offset
	? 1.0f
	: (1.0f + std::sin((width / offset) + phase)) / 2.0f;
}

template <typename T>
RippleCurve<T>::RippleCurve(
    float position, float width_) : T(position), width(width_) {}
template RippleCurve<>::RippleCurve(float, float);
template RippleCurve<Dial>::RippleCurve(float, float);

static float sinc(float x) {
    if (0.0f == x) return 1.0f;
    return std::sin(x) / x;
}

template <typename T>
float RippleCurve<T>::operator()(float place) const {
    float offset = pi * std::abs(T::operator()(place) * 3.0f / width);
    if (1.0f / 2.0f > offset) {
	return sinc(offset);
    } else {
	return (1.0f + std::sin(offset)) / (2.0f * offset);
    }
}
template float RippleCurve<>::operator()(float place) const;
template float RippleCurve<Dial>::operator()(float place) const;
