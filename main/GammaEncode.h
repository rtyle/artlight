#pragma once

#include <cstdint>
#include <limits>

/// GammaEncode is a function object
/// that is constructed with a gamma correction value
/// and may be used to encode gamma corrected uint8_t values.
class GammaEncode {
public:
    static auto constexpr size = 1 + std::numeric_limits<uint8_t>::max();
private:
    uint8_t curve[size];
public:
    GammaEncode(float gamma);
    uint8_t operator()(uint8_t value) const {return curve[value];}
};
