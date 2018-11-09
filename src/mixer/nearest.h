#ifndef MIXER_NEAREST_H_
#define MIXER_NEAREST_H_

#include "interpolator.h"

class NearestNeighbor : public Interpolator {
public:
    std::string name() { return "nearest"; }
    int32_t sample(uint16_t t) { return b0; }
};

#endif  // MIXER_NEAREST_H_
