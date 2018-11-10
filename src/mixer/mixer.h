#ifndef HAZE_MIXER_MIXER_H_
#define HAZE_MIXER_MIXER_H_

#include <vector>
#include <array>
#include <memory>
#include <mixer/sample.h>
#include <mixer/channel.h>


class Mixer {
    int rate;
    std::vector<Sample> sample;
    Channel *channel;
public:
    Mixer(int);
    template<typename T>
    void mix(T *, size_t);               // mix sample data
    void load_sample(void *, uint32_t);  // load sample data
    void set_sample(int, int);           // set active sample
    void start(int, uint32_t);           // set sample start offset
    void end(int, uint32_t);             // set sample end offset
    void loop_start(int, uint32_t);      // set loop start offset
    void loop_end(int, uint32_t);        // set loop end offset
    int num_channels();                  // get number of mixer channels
};  // Mixer


#endif  // HAZE_MIXER_MIXER_H_


