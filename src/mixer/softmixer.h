#ifndef HAZE_MIXER_SOFTMIXER_H_
#define HAZE_MIXER_SOFTMIXER_H_

#include <vector>
#include <array>
#include <memory>
#include "mixer/mixer.h"
#include "mixer/sample.h"
#include "mixer/softmixer_voice.h"
#include "mixer/interpolator.h"
#include "util/options.h"


class SoftMixer : public Mixer {
    std::vector<Sample> sample;
    std::vector<Voice *> voice;
public:
    SoftMixer(int, int, Options);
    ~SoftMixer();
    void mix(int16_t *, int) override;       // mix sample data
    void mix(float *, int) override;
    void reset();
    void add_sample(void *, uint32_t, double = 1.0, uint32_t = 0); // load sample data
    void set_sample(int, int);               // set active sample
    void set_start(int, uint32_t);           // set sample start offset
    void set_end(int, uint32_t);             // set sample end offset
    void set_loop_start(int, uint32_t);      // set loop start offset
    void set_loop_end(int, uint32_t);        // set loop end offset
    void enable_loop(int, bool, bool=false); // enable or disable sample loop
    void set_volume(int, int);               // set voice volume
    void set_pan(int, int);                  // set voice pan
    void set_voicepos(int, double);          // set voice position
    void set_period(int, double);            // set period
};  // SoftMixer


#endif  // HAZE_MIXER_SOFTMIXER_H_


