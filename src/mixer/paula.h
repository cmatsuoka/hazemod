#ifndef HAZE_MIXER_PAULA_H_
#define HAZE_MIXER_PAULA_H_

#include <cstdint>
#include <cstring>

// A very simple Paula simulator

// Audio registers
constexpr uint32_t AUD0LCH = 0xdff0a0;  // Audio channel 0 location
constexpr uint32_t AUD0LEN = 0xdff0a4;  // Audio channel 0 length
constexpr uint32_t AUD0PER = 0xdff0a6;  // Audio channel 0 period
constexpr uint32_t AUD0VOL = 0xdff0a8;  // Audio channel 0 volume
constexpr uint32_t AUD1LCH = 0xdff0b0;  // Audio channel 1 location
constexpr uint32_t AUD1LEN = 0xdff0b4;  // Audio channel 1 length
constexpr uint32_t AUD1PER = 0xdff0b6;  // Audio channel 1 period
constexpr uint32_t AUD1VOL = 0xdff0b8;  // Audio channel 1 volume
constexpr uint32_t AUD2LCH = 0xdff0c0;  // Audio channel 2 location
constexpr uint32_t AUD2LEN = 0xdff0c4;  // Audio channel 2 length
constexpr uint32_t AUD2PER = 0xdff0c6;  // Audio channel 2 period
constexpr uint32_t AUD2VOL = 0xdff0c8;  // Audio channel 2 volume
constexpr uint32_t AUD3LCH = 0xdff0d0;  // Audio channel 3 location
constexpr uint32_t AUD3LEN = 0xdff0d4;  // Audio channel 3 length
constexpr uint32_t AUD3PER = 0xdff0d6;  // Audio channel 3 period
constexpr uint32_t AUD3VOL = 0xdff0d8;  // Audio channel 3 volume

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


class Paula {
    int rate_;
    uint16_t reg_[64];  // audio registers starting at 0xdff0a0
    bool cia_led_;      // CIAA led setting (0BFE001 bit 1)

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
    
public:
    Paula(int sr) :
        rate_(sr),
        reg_{0},
        cia_led_(false),
        global_output_level(0),
        active_bleps(0),
        remainder(PAULA_HZ / rate_),
        fdiv(PAULA_HZ / rate_)
    {
        memset(bleps, 0, sizeof(bleps));
    }
    ~Paula() {}
    void mix(int16_t *, int);                // mix sample data
    void mix(float *, int);
    void write_w(const uint32_t addr, const uint16_t val) {
        reg_[addr - AUD0LCH] = val;
    }
    void write_l(const uint32_t addr, const uint32_t val) {
        reg_[addr - AUD0LCH    ] = val >> 16;
        reg_[addr - AUD0LCH + 1] = val & 0xffff;
    }
    void set_cia_led(const bool val) { cia_led_ = val; }
    bool cia_led() { return cia_led_; }
    int16_t output_sample();
    void input_sample(const int16_t);
    void do_clock(int16_t);
};


#endif  // HAZE_MIXER_PAULA_H_
