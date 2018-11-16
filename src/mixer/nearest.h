#ifndef MIXER_NEAREST_H_
#define MIXER_NEAREST_H_

#include "mixer/interpolator.h"


class NearestInterpolator : public Interpolator {
public:
    std::string name() override { return "nearest"; }
    int32_t get(uint16_t t) override { return b0; }
};

#endif  // MIXER_NEAREST_H_
