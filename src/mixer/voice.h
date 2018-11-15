#ifndef HAZE_MIXER_VOICE_H_
#define HAZE_MIXER_VOICE_H_

#include <algorithm>
#include <cstdint>
#include "mixer/sample.h"
#include "mixer/interpolator.h"


class Voice {
    int num_;
    uint32_t pos_;
    uint32_t frac_;
    double step_;
    int volume_;
    int pan_;
    bool mute_;            // channel is muted
    bool loop_;            // sample has loop
    bool finish_;          // single-shot sample finished
    bool bidir_;           // loop is bidirectional
    bool forward_;         // current loop direction
    uint32_t start_;
    uint32_t end_;
    uint32_t loop_start_;
    uint32_t loop_end_;
    Sample& smp_;
    Interpolator *itp_;

protected:
    void add_step() {
        // TODO: handle bidir loops
        frac_ += static_cast<uint32_t>((1 << 16) * step_);
        pos_ += frac_ >> 16;
        frac_ &= (1 << 16) - 1;

        if (loop_) {
            if (pos_ >= loop_end_) {
                pos_ -= (loop_end_ - loop_start_);
            }
        } else {
            if (pos_ >= end_) {
                finish_ = true;
            }
        }
    }

public:
    Voice(int, InterpolatorType);
    ~Voice();

    uint16_t do_sample() {
        uint32_t val = itp_->sample(frac_);
        uint32_t prev = pos_;
        add_step();
        if (prev != pos_) {
            itp_->add(smp_.get(pos_));
        }
        return (val * volume_) >> 10;
    }

    uint32_t pos() { return pos_; }

    int frac() { return int(double(1 << 16) * (pos_ - int(pos_))); }

    void set_start(uint32_t val) { start_ = pos_ = val; }
    void set_end(uint32_t val) { end_ = val; }
    void set_loop_start(uint32_t val) { loop_start_ = val; }
    void set_loop_end(uint32_t val) { loop_end_ = val; }
    void enable_loop(bool val) { loop_ = val; }
    void set_volume(int val) { volume_ = std::clamp(val, 0, 1023); }
    void set_pan(int val) { pan_ = std::clamp(val, -127, 128); }
    void set_smp(Sample& smp) { smp_ = smp; }

    void set_step(double val) {
        step_ = val;
    }

    int volume() { return volume_; }
    int pan() { return pan_; }
    double step() { return step_; }
    bool loop() { return loop_; }
    void set_interpolator(InterpolatorType);
    Sample& smp() { return smp_; }
};

#endif  // HAZE_MIXER_VOICE_H_
