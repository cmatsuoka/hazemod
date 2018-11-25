#ifndef HAZE_MIXER_PAULA_H_
#define HAZE_MIXER_PAULA_H_

#include <cstdint>
#include <cstring>
#include "mixer/mixer.h"
#include "mixer/paula_channel.h"
#include "util/databuffer.h"


// 131072 to 0, 2048 entries
constexpr float PAULA_HZ  = 3546895.0;
constexpr int MINIMUM_INTERVAL = 16;

constexpr uint32_t BLEP_SCALE = 17;
constexpr int BLEP_SIZE = 2048;
constexpr int MAX_BLEPS = (BLEP_SIZE / MINIMUM_INTERVAL);


// A500 output simulator
// Based on Antti S. Lankila's reference implementation
class Simulator {

    struct Blep {
        int16_t level;
        int16_t age;
    };

    int rate_;

    // the instantenous value of Paula output
    int16_t global_output_level;

    // count of simultaneous bleps to keep track of
    int active_bleps;

    // place to keep our bleps in. MAX_BLEPS should be
    // defined as a BLEP_SIZE / MINIMUM_EVENT_INTERVAL.
    // For Paula, minimum event interval could be even 1, but it makes
    // sense to limit it to some higher value such as 16.
    Blep bleps[MAX_BLEPS];

public:
    Simulator(const int sr) :
        rate_(sr),
        global_output_level(0),
        active_bleps(0)
    {
        memset(bleps, 0, sizeof(bleps));
    }

    int16_t output_sample(bool const&);
    void input_sample(const int16_t);
    void do_clock(int16_t);
};


class Paula : public Mixer {
    int rate_;
    PaulaChannel *channel_[4];
    bool cia_led_;      // CIAA led setting (0BFE001 bit 1)

    DataBuffer data_;
    Simulator simr;
    Simulator siml;

    float remainder;
    float fdiv;

public:
    Paula(void *ptr, uint32_t size, int sr) :
        Mixer(4, sr),
        rate_(sr),
        cia_led_(false),
        data_(ptr, size),
        simr(sr),
        siml(sr),
        remainder(PAULA_HZ / rate_),
        fdiv(remainder)
    {
        channel_[0] = new PaulaChannel(data_, sr);
        channel_[1] = new PaulaChannel(data_, sr);
        channel_[2] = new PaulaChannel(data_, sr);
        channel_[3] = new PaulaChannel(data_, sr);
    }

    ~Paula() {
        delete channel_[3];
        delete channel_[2];
        delete channel_[1];
        delete channel_[0];
    }

    void mix(int16_t *, int);                // mix sample data
    void mix(float *, int);
    void reset();
    void set_cia_led(const bool val) { cia_led_ = val; }
    bool cia_led() { return cia_led_; }
    void set_start(const int chn, const uint32_t val) { channel_[chn]->set_start(val); }
    void set_length(const int chn, const uint16_t val) { channel_[chn]->set_length(val); }
    void set_period(const int chn, const uint16_t val) { channel_[chn]->set_period(val); }
    void set_volume(const int chn, const uint16_t val) { channel_[chn]->set_volume(val); }
    void set_pan(const int chn, const int8_t val) { channel_[chn]->set_pan(val); }
    void start_dma(const uint16_t);
    void stop_dma(const uint16_t);
    void enable_filter(bool val) { cia_led_ = val; }
};


#endif  // HAZE_MIXER_PAULA_H_
