#pragma once

/// Dial is used to model the notion of circular dial position [0.0f, 1.0f).
/// It is a function object that maps a place on the circle [0.0f, 1.0f)
/// to the centered relative offset [-0.5f, 0.5f) from the dial position.
class Dial {
private:
    float const	position;
public:
    Dial(float position = 0.0f);
    virtual float operator()(float place) const;
};

/// BellDial is a Dial whose function object composes
/// a rectified cosine function (bump)
/// after the Dial position offset.
class BumpDial : public Dial {
private:
    float const width;
public:
    BumpDial(float position = 0.0f, float width = 1.0f);
    float operator()(float place) const override;
};

/// BellDial is a Dial whose function object composes
/// a normal distribution function (bell)
/// (http://wikipedia.org/wiki/Normal_distribution)
/// after the Dial position offset.
class BellDial : public Dial {
private:
    float const twoSigmaSquared;
public:
    BellDial(float position = 0.0f, float sigma = 1.0f);
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

/// BellStandingWaveDial is a BellDial whose function object composes
/// the BellDial after the average of opposing WaveDial function objects.
class BellStandingWaveDial : public BellDial {
private:
    WaveDial const rightWaveDial;
    WaveDial const leftWaveDial;
public:
    BellStandingWaveDial(
	float position, float sigma, float wavePosition, float waveWidth);
    float operator()(float place) const override;

};

/// RippleDial is a Dial whose function object composes
/// a modified sinc function (sin(x)/x)
/// https://www.desmos.com/calculator/9ndsrd9psf
/// https://en.wikipedia.org/wiki/Sinc_function
/// after the Dial position offset
class RippleDial : public Dial {
private:
    float const width;
public:
    RippleDial(float position, float width);
    float operator()(float place) const override;
};
