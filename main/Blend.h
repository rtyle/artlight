#pragma once

/// Blend is a function (object) that blends two values (of type T)
/// using linear interpolation
template <typename T> class Blend {
private:
    T const a;
    T const b;
    T const slope = b - a;
public:
    Blend(T a_, T b_) : a(a_), b(b_) {}
    template <typename F> T operator ()(F f) const {return slope * f + a;}
};
