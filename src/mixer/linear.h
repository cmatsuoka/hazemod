#ifndef MIXER_LINEAR_H
#define MIXER_LINEAR_H_

#include "mixer/interpolator.h"


class LinearInterpolator : public Interpolator {
public:
    std::string name() override { return "linear"; }
    int32_t sample(uint16_t t) override { return b1 + (((b0 - b1) * t) >> SMIX_SHIFT); }
};

#endif  // MIXER_LINEAR_H_
