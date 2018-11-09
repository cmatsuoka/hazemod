#ifndef MIXER_LINEAR_H
#define MIXER_LINEAR_H_

#include "interpolator.h"


class LinearInterpolator : public Interpolator {
public:
    std::string name() { return "linear"; }
    int32_t sample(uint16_t t) { return b1 + (((b0 - b1) * t) >> SMIX_SHIFT); }
};

#endif  // MIXER_LINEAR_H_
