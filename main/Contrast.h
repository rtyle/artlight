#pragma once

/// Contrast is a monotonically increasing function (object)
/// that, with a curvature of 0, is the Identity function
/// but, as the magnitude of curvature increases,
/// becomes an increasingly S-shaped contrast curve
/// mapping the domain [0, 1] to the range [0, 1].
/// This function increases the contrast of the midtones
/// at the expense of the blacks and whites.
/// Similar functions are used in image processing programs.
/// https://www.desmos.com/calculator/2wgzlu8bqx
struct Contrast {
private:
    float curvature;
    float normalize;
public:
    float getCurvature() const;
    void setCurvature(float curvature);
    Contrast(float curvature_ = 0.0f);
    float operator () (float x) const;
};
