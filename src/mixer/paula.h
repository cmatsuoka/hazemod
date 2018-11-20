#ifndef HAZE_MIXER_PAULA_H_
#define HAZE_MIXER_PAULA_H_

#include <cstdint>
#include <cstring>
#include <mixer/sample.h>
#include "util/databuffer.h"

// A very simple Paula simulator

// 131072 to 0, 2048 entries
constexpr double PAULA_HZ  = 3546895.0;
constexpr int MINIMUM_INTERVAL = 16;

constexpr uint32_t BLEP_SCALE = 17;
constexpr int BLEP_SIZE = 2048;
constexpr int MAX_BLEPS = (BLEP_SIZE / MINIMUM_INTERVAL);


struct Blep {
    int16_t level;
    int16_t age;
};

struct PaulaChannel {
    uint32_t pos;
    uint32_t frac;
    uint32_t audloc;
    uint16_t audlen;
    uint16_t audper;
    uint16_t audvol;
};

class Paula {
    int rate_;
    PaulaChannel channel_[4];
    bool cia_led_;      // CIAA led setting (0BFE001 bit 1)

    DataBuffer data_;

    // the instantenous value of Paula output
    int16_t global_output_level;

    // count of simultaneous bleps to keep track of
    int active_bleps;

    // place to keep our bleps in. MAX_BLEPS should be
    // defined as a BLEP_SIZE / MINIMUM_EVENT_INTERVAL.
    // For Paula, minimum event interval could be even 1, but it makes
    // sense to limit it to some higher value such as 16.
    Blep bleps[MAX_BLEPS];

    double remainder;
    double fdiv;
    
    int16_t sample_from_voice(int);

public:
    Paula(void *ptr, uint32_t size, int sr) :
        rate_(sr),
        cia_led_(false),
        data_(ptr, size),
        global_output_level(0),
        active_bleps(0),
        remainder(PAULA_HZ / rate_),
        fdiv(PAULA_HZ / rate_)
    {
        memset(channel_, 0, sizeof(channel_));
        memset(bleps, 0, sizeof(bleps));
    }
    ~Paula() {}
    void mix(int16_t *, int);                // mix sample data
    void mix(float *, int);
    void set_cia_led(const bool val) { cia_led_ = val; }
    bool cia_led() { return cia_led_; }
    int16_t output_sample();
    void input_sample(const int16_t);
    void do_clock(int16_t);
};


#endif  // HAZE_MIXER_PAULA_H_
