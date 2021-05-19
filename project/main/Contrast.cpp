#include <cmath>

#include "Contrast.h"

float constexpr pi	{std::acos(-1.0f)};
float constexpr tau	{2.0f * pi};

float Contrast::getCurvature() const {
    return curvature;
}

void Contrast::setCurvature(float curvature_) {
    if ((curvature = curvature_)) {
	normalize = -1.0f / (2.0f * std::atan(-curvature / tau));
    }
}

Contrast::Contrast(float curvature) {
    setCurvature(curvature);
}

float Contrast::operator()(float x) const {
    return curvature
	? 0.5f + normalize * std::atan(curvature * (x - 0.5f) / pi)
	: x;
}
