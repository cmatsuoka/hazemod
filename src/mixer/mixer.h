#ifndef HAZE_MIXER_MIXER_H_
#define HAZE_MIXER_MIXER_H_

#include <vector>
#include <array>
#include <memory>
#include "mixer/sample.h"
#include "mixer/channel.h"


class Mixer {
    int srate;
    int num_channels;
    std::vector<Sample> sample;
    std::vector<Channel> channel;
public:
    Mixer(int, int);
    void mix(int16_t *, int);                // mix sample data
    int rate() { return srate; }
    void add_sample(void *, uint32_t);       // load sample data
    void set_sample(int, int);               // set active sample
    void set_start(int, uint32_t);           // set sample start offset
    void set_end(int, uint32_t);             // set sample end offset
    void set_loop_start(int, uint32_t);      // set loop start offset
    void set_loop_end(int, uint32_t);        // set loop end offset
    int channels() { return num_channels; }  // get number of mixer channels
    void set_volume(int, int);               // set channel volume
    void set_pan(int, int);                  // set channel pan
    void set_voicepos(int, double);          // set voice position
    void set_period(int, double);            // set period
    void enable_filter(bool);
};  // Mixer


#endif  // HAZE_MIXER_MIXER_H_


