#ifndef HAZE_MIXER_MIXER_H_
#define HAZE_MIXER_MIXER_H_

#include <vector>
#include <array>
#include <memory>
#include "mixer/sample.h"
#include "mixer/voice.h"

// Amiga PAL color carrier frequency (PCCF) = 4.43361825 MHz
// Amiga CPU clock = 1.6 * PCCF = 7.0937892 MHz
constexpr double C4PalRate = 8287.0;   // 7093789.2 / period (C4) * 2
constexpr double C4Period = 428.0;
constexpr double PalRate = 250.0;
constexpr uint32_t Lim16_lo = -32768;
constexpr uint32_t Lim16_hi = 32767;


class Mixer {
    int srate;
    int num_voices;
    std::vector<Sample> sample;
    std::vector<Voice *> voice;
public:
    Mixer(int, int);
    ~Mixer();
    void mix(int16_t *, int);                // mix sample data
    void mix(float *, int);                  //
    int rate() { return srate; }
    void add_sample(void *, uint32_t);       // load sample data
    void set_sample(int, int);               // set active sample
    void set_start(int, uint32_t);           // set sample start offset
    void set_end(int, uint32_t);             // set sample end offset
    void set_loop_start(int, uint32_t);      // set loop start offset
    void set_loop_end(int, uint32_t);        // set loop end offset
    void enable_loop(int, bool);             // enable or disable sample loop
    int voices() { return num_voices; }      // get number of mixer voices
    void set_volume(int, int);               // set voice volume
    void set_pan(int, int);                  // set voice pan
    void set_voicepos(int, double);          // set voice position
    void set_period(int, double);            // set period
    void enable_filter(bool);
};  // Mixer


#endif  // HAZE_MIXER_MIXER_H_


