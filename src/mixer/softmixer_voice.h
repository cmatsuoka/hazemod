#ifndef HAZE_MIXER_VOICE_H_
#define HAZE_MIXER_VOICE_H_

#include <algorithm>
#include <cstdint>
#include "mixer/sample.h"
#include "mixer/interpolator.h"


class Voice {
    int num_;
    bool enable_;
    uint32_t pos_;
    uint32_t frac_;
    double step_;
    int volume_;
    int pan_;
    bool mute_;            // channel is muted
    bool loop_;            // sample has loop
    bool bidir_;           // loop is bidirectional
    bool dir_;             // loop direction
    //bool finish_;        // single-shot sample finished
    bool forward_;         // current loop direction
    uint32_t start_;
    uint32_t end_;
    uint32_t loop_start_;
    uint32_t loop_end_;
    uint32_t prev_;
    Sample sample_;
    Interpolator *itp_;

    void fix_loop() {
        if (loop_start_ >= end_) {
            loop_start_ = end_ - 1;
        }
        if (loop_end_ > end_) {
            loop_end_ = end_;
        }
        if (loop_end_ <= loop_start_) {
            loop_end_ = loop_start_ + 1;
        }
    }

protected:
    void add_step() {
        frac_ += static_cast<uint32_t>((1 << 16) * step_);
        if (!dir_) {
            pos_ += frac_ >> 16;
        } else {
            pos_ -= frac_ >> 16;
        }
        frac_ &= (1 << 16) - 1;

        if (loop_) {
            if (!bidir_) {
                const uint32_t loop_len = loop_end_ - loop_start_;
                while (pos_ >= loop_end_) {
                    pos_ -= loop_len;
                }
            } else if (!dir_) {
                if (pos_ >= loop_end_) {
                    // forward loop
                    const uint32_t loop_len = loop_end_ - loop_start_;
                    do {
                        pos_ -= loop_len;
                    } while (pos_ >= loop_end_);
                    pos_ = loop_end_ - (pos_ - loop_start_) - 1;
                    dir_ = true;
                }
            } else {
                if (pos_ < loop_start_) {
                    // backward loop
                    const uint32_t loop_len = loop_end_ - loop_start_;
                    do {
                        pos_ += loop_len;
                    } while (pos_ < loop_start_);
                    pos_ = loop_start_ + (loop_end_ - pos_) - 1;
                    dir_ = false;
                }
            }
        }
    }

public:
    Voice(int, InterpolatorType);
    ~Voice();

    int num() { return num_; }

    void reset();

    int32_t get() {
        if (pos_ >= end_) {
            return 0;
        }
        if (prev_ != pos_) {
            itp_->put(sample_.get(pos_));
            prev_ = pos_;
        }
        int32_t y = itp_->get(frac_>>1);
        add_step();
        return y;
    }

    uint32_t pos() { return pos_; }
    uint32_t frac() { return frac_; }

    void set_start(uint32_t val) { start_ = pos_ = val; }
    void set_end(uint32_t val) { end_ = val; }
    void set_loop_start(uint32_t val) { loop_start_ = val; fix_loop(); }
    void set_loop_end(uint32_t val) { loop_end_ = val; fix_loop(); }
    void enable_loop(bool val, bool bidir = false) { loop_ = val; bidir_ = bidir; }
    void set_volume(int val) { volume_ = std::clamp(val, 0, 256); }
    void set_pan(int val) { pan_ = std::clamp(val, -128, 127); }
    void set_sample(Sample const& sample) { sample_ = sample; }
    void set_voicepos(double d) { pos_ = uint32_t(d); frac_ = uint32_t(double(1 << 16) * (d - int(d))); }
    void set_step(double val) { step_ = val; }
    void set_enable(bool val) { enable_ = val; }
    int volume() { return volume_; }
    int pan() { return pan_; }
    double step() { return step_; }
    bool enable() { return enable_; }
    bool loop() { return loop_; }
    bool bidir() { return bidir_; }
    void set_interpolator(InterpolatorType);
    Sample& sample() { return sample_; }
};

#endif  // HAZE_MIXER_VOICE_H_
