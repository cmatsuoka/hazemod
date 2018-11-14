#ifndef HAZE_MIXER_CHANNEL_H_
#define HAZE_MIXER_CHANNEL_H_

#include <algorithm>
#include <cstdint>
#include "mixer/sample.h"
#include "mixer/interpolator.h"


class Channel {
    uint32_t pos_;
    uint16_t frac_;
    double period_;
    int volbase_;
    int volume_;
    int pan_;
    bool mute_;            // channel is muted
    bool loop_;            // sample has loop
    bool finish_;          // single-shot sample finished
    bool bidir_;           // loop is bidirectional
    bool forward_;         // current loop direction
    uint32_t loop_start_;
    uint32_t loop_end_;
    uint32_t end_;
    Sample& smp_;
    Interpolator *itp_;

protected:
    void add_step() {
        // TODO: handle bidir loops
        frac_ += (1 << 16) * period_;
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
    Channel();

    uint16_t sample() {
        uint16_t val = itp_->sample(frac_); 
        uint32_t prev = pos_;
        add_step();
        if (prev != pos_) {
            itp_->add(smp_.get(pos_));
        }
        return val;
    }

    uint32_t pos() { return pos_; }

    int frac() { return int(double(1 << 16) * (pos_ - int(pos_))); }

    void set_volume(int val) { volume_ = std::clamp(val, 0, volbase_); }
    void set_pan(int val) { pan_ = std::clamp(val, -127, 128); }
    void set_period(double val) { period_ = val; }
    int volume() { return volume_; }
    int pan() { return pan_; }
    double period() { return period_; }
    bool loop() { return loop_; }
};

#endif  // HAZE_MIXER_CHANNEL_H_
