#include "mixer/mixer.h"
#include <memory>

constexpr int DefaultRate = 44100;


Mixer::Mixer(int num)
{
    num_channels = num;
    channel = new Channel[num];
}

void Mixer::set_pan(int chn, int val)
{
    if (chn >= num_channels) {
        return;
    }
}
