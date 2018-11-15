#include "mixer/voice.h"
#include <stdexcept>
#include "mixer/nearest.h"
#include "mixer/linear.h"


Voice::Voice(int num, InterpolatorType typ) :
    num_(num),
    pos_(0),
    frac_(0),
    step_(0.0),
    volume_(1023),
    pan_(0),
    mute_(false),
    loop_(false),
    finish_(false),
    start_(0),
    end_(0),
    loop_start_(0),
    loop_end_(0),
    prev_(-1),
    smp_(empty_sample),
    itp_(nullptr)
{
    set_interpolator(typ);
};

Voice::~Voice()
{
    delete itp_;
}

void Voice::set_interpolator(InterpolatorType typ)
{
    if (itp_ != nullptr) {
        delete itp_;
    }

    switch (typ) {
    case NearestNeighborInterpolatorType:
        itp_ = new NearestNeighbor();
        break;
    case LinearInterpolatorType:
        itp_ = new LinearInterpolator();
        break;
    default:
        throw std::runtime_error("invalid interpolator type");
    }
}
