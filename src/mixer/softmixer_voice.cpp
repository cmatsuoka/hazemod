#include "mixer/softmixer_voice.h"
#include <stdexcept>
#include "mixer/nearest.h"
#include "mixer/linear.h"
#include "mixer/spline.h"


Voice::Voice(int num, InterpolatorType typ) :
    num_(num),
    enable_(false),
    pos_(0),
    frac_(0),
    step_(0.0),
    volume_(256),
    pan_(0),
    mute_(false),
    loop_(false),
    bidir_(false),
    dir_(false),
    //finish_(false),
    forward_(true),
    start_(0),
    end_(0),
    loop_start_(0),
    loop_end_(0),
    prev_(-1),
    sample_(empty_sample),
    itp_(nullptr)
{
    set_interpolator(typ);
};

Voice::~Voice()
{
    delete itp_;
}

void Voice::reset()
{
    pos_ = 0;
    frac_ = 0;
    step_ = 0.0;
    volume_ = 256;
    pan_ = 0;
    mute_ = false;
    loop_ = false;
    bidir_ = false;
    dir_ = false;
    forward_ = true;
    start_ = 0;
    end_ = 0;
    loop_start_ = 0;
    loop_end_ = 0;
    prev_ = -1;
    sample_ = empty_sample;
}

void Voice::set_interpolator(InterpolatorType typ)
{
    delete itp_;

    switch (typ) {
    case NearestInterpolatorType:
        itp_ = new NearestInterpolator();
        break;
    case LinearInterpolatorType:
        itp_ = new LinearInterpolator();
        break;
    case SplineInterpolatorType:
        itp_ = new SplineInterpolator();
        break;
    default:
        itp_ = nullptr;
        throw std::runtime_error("invalid interpolator type");
    }
}
