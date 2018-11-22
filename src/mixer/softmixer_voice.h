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
    //bool finish_;        // single-shot sample finished
    bool bidir_;           // loop is bidirectional
    bool forward_;         // current loop direction
    uint32_t start_;
    uint32_t end_;
    uint32_t loop_start_;
    uint32_t loop_end_;
    uint32_t prev_;
    Sample& sample_;
    Interpolator *itp_;

protected:
    void add_step() {
        // TODO: handle bidir loops
        frac_ += static_cast<uint32_t>((1 << 16) * step_);
        pos_ += frac_ >> 16;
        frac_ &= (1 << 16) - 1;

        if (loop_) {
            if (loop_end_ > loop_start_) {
                while (pos_ >= loop_end_) {
                    pos_ -= (loop_end_ - loop_start_);
                }
            }
        }
    }

public:
    Voice(int, InterpolatorType);
    ~Voice();

    int num() { return num_; }

    int32_t get() {
        if (pos_ >= end_) {
            return 0;
        }
        if (prev_ != pos_) {
            auto x = sample_.get(pos_);
            itp_->put(x);
            prev_ = pos_;
        }
        int32_t y = itp_->get(frac_);
        add_step();
        return y;
    }

    uint32_t pos() { return pos_; }
    uint32_t frac() { return frac_; }

    void set_start(uint32_t val) { start_ = pos_ = val; }
    void set_end(uint32_t val) { end_ = val; }
    void set_loop_start(uint32_t val) { loop_start_ = val; }
    void set_loop_end(uint32_t val) { loop_end_ = val; }
    void enable_loop(bool val) { loop_ = val; }
    void set_volume(int val) { volume_ = std::clamp(val, 0, 256); }
    void set_pan(int val) { pan_ = std::clamp(val, -127, 128); }
    void set_sample(Sample& sample) { sample_ = sample; }
    void set_voicepos(double d) { pos_ = uint32_t(d); frac_ = uint32_t(double(1 << 16) * (d - int(d))); }
    void set_step(double val) { step_ = val; }
    int volume() { return volume_; }
    int pan() { return pan_; }
    double step() { return step_; }
    bool loop() { return loop_; }
    void set_interpolator(InterpolatorType);
    Sample& sample() { return sample_; }
};

#endif  // HAZE_MIXER_VOICE_H_