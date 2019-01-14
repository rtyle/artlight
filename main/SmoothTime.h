#pragma once

#include <cstdint>
#include <ctime>

class SmoothTime {
private:
    char const *	name;
    size_t const	stepCount;
    size_t		stepLeft;
    float		stepProduct;
    float		stepFactor;
    int64_t		lastBootTime;
public:
    SmoothTime(char const * name, size_t count);
    int64_t	microsecondsSinceEpoch();
    uint32_t	millisecondsSinceTwelveLocaltime();
};
