#include "GammaEncode.h"

#include <cmath>

void GammaEncode::gamma(float value) {
    float max = size - 1;
    uint8_t i = 0;
    for (auto & e: curve) {
	e = 0.5 + max * std::pow(i++ / max, value);
    }
}

GammaEncode::GammaEncode(float value) {
    gamma(value);
}
