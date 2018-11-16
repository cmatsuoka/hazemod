#ifndef MIXER_INTERPOLATOR_H_
#define MIXER_INTERPOLATOR_H_

#include <string>
#include <cstdint>

#define SMIX_SHIFT 16

enum InterpolatorType {
    NearestInterpolatorType,
    LinearInterpolatorType,
    SplineInterpolatorType
};


class Interpolator {
protected:
    int32_t b0, b1, b2;
public:
    Interpolator(): b0(0), b1(0), b2(0) {}
    virtual ~Interpolator() {}
    virtual int32_t get(uint16_t t) = 0;
    virtual std::string name() = 0;
    void put(int16_t y) { b2 = b1; b1 = b0; b0 = y; }
};

#endif  // MIXER_INTERPOLATOR_H_
