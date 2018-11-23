#ifndef HAZE_MIXER_MIXER_H_
#define HAZE_MIXER_MIXER_H_

#include "mixer/interpolator.h"


// Amiga PAL color carrier frequency (PCCF) = 4.43361825 MHz
// Amiga CPU clock = 1.6 * PCCF = 7.0937892 MHz
constexpr double C4PalRate = 8287.0;   // 7093789.2 / period (C4) * 2
constexpr double C4Period = 428.0;
constexpr double PalRate = 250.0;


class Mixer {
protected:
    int srate;
    int num_voices;
public:
    Mixer(int num, int sr, InterpolatorType = NearestInterpolatorType) :
        srate(sr),
        num_voices(num)
    {}
    virtual ~Mixer() {}
    virtual void mix(int16_t *, int) = 0;  // mix sample data
    virtual void mix(float *, int) = 0;
    virtual void reset() = 0;
    int rate() { return srate; }
    int voices() { return num_voices; }
};  // Mixer


#endif  // HAZE_MIXER_MIXER_H_


