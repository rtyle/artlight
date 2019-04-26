#pragma once

/// Curve is used to model the notion of a curve at a position.
/// It is a function object that maps a place
/// to the relative offset from its position.
class Curve {
protected:
    float const position;
public:
    Curve(float position_ = 0.0f);
    virtual ~Curve();
    virtual float operator()(float place) const;
};

/// Dial is used to model the notion of curve at a position
/// on a circular dial [0.0f, 1.0f).
/// It is a function object that maps a place on the circle [0.0f, 1.0f)
/// to the relative offset [-0.5f, 0.5f) from its position.
class Dial : public Curve {
public:
    Dial(float position = 0.0f);
    float operator()(float place) const override;
};

/// BumpCurve is a Curve whose function object composes
/// a modified 1 - x**2 function (one non-negative bump of width)
/// after the Curve position offset.
class BumpCurve : Curve {
private:
    float const width;
public:
    BumpCurve(float position = 0.0f, float width_ = 1.0f);
    float operator()(float place) const override;
};

/// BumpsCurve is a Curve whose function object composes
/// a rectified cosine function (repeating bumps of width)
/// after the Curve position offset.
class BumpsCurve : Curve {
private:
    float const width;
public:
    BumpsCurve(float position = 0.0f, float width_ = 1.0f);
    float operator()(float place) const override;
};

/// BellCurve is a Curve (of type T) whose function object composes
/// a normal distribution function (bell)
/// (http://wikipedia.org/wiki/Normal_distribution)
/// after the Curve (of type T) position offset.
/// Width is 2 * sigma.
template <typename T = Curve>
class BellCurve : public T {
private:
    float const twoSigmaSquared;
public:
    BellCurve(float position = 0.0f, float width /* 2 * sigma */ = 1.0f);
    float operator()(float place) const override;
};

/// WaveDial is a Dial whose function object composes
/// a modified cosine function (wave)
/// after the Dial position offset
class WaveDial : public Dial {
private:
    float const width;
public:
    WaveDial(float position = 0.0f, float width = 1.0f);
    float operator()(float place) const override;
};

/// BellStandingWaveDial is a BellCurve<Dial> whose function object composes
/// the BellCurve<Dial> after the average of opposing WaveDial function objects.
class BellStandingWaveDial : public BellCurve<Dial> {
private:
    WaveDial const rightWaveDial;
    WaveDial const leftWaveDial;
public:
    BellStandingWaveDial(
	float position, float sigma, float wavePosition, float waveWidth);
    float operator()(float place) const override;

};

/// BloomCurve is a Curve whose function object is
/// a modified sine(width/place + phase) function (bloom),
/// offset at position.
/// https://www.desmos.com/calculator/n0uxcq9m5f
/// Width measures the distance between the first and the last peaks (petals)
/// when phase is 0.
/// The peaks (petals) get closer and closer as curve's position is approached.
/// Increasing the phase will cause the peaks to move outward from this
/// position; decreasing the phase will cause them to move inward.
class BloomCurve : public Curve {
private:
    float const width;
    float const phase;
public:
    BloomCurve(float position = 0.0f, float width = 1.0f, float phase = 0.0f);
    float operator()(float place) const override;
};

/// RippleCurve is a Curve (of type T) whose function object composes
/// a modified sinc function (sin(x)/x)
/// https://www.desmos.com/calculator/9ndsrd9psf
/// https://en.wikipedia.org/wiki/Sinc_function
/// after the Curve position offset.
/// A width of 1 on a Dial results in a ripple with one bump.
template <typename T = Curve>
class RippleCurve : public T {
private:
    float const width;
public:
    RippleCurve(float position, float width);
    float operator()(float place) const override;
};

/// SawtoothCurve is a Curve whose function object defined by its period.
/// Its output ranges from 0 to <1.
class SawtoothCurve : public Curve {
private:
    float period;
public:
    SawtoothCurve(float position = 0.0f, float period = 1.0f);
    float operator()(float place) const override;
};
