#ifndef MIXER_SPLINE_H_
#define MIXER_SPLINE_H_

#include "mixer/interpolator.h"

#define SPLINE_SHIFT 14


class SplineInterpolator : public Interpolator {
public:
    std::string name() override { return "spline"; }
    int32_t get(uint16_t t) override;
};

#endif  // MIXER_SPLINE_H_
