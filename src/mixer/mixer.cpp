#include "mixer/mixer.h"
#include <memory>

constexpr int DefaultRate = 44100;


Mixer::Mixer(int num)
{
    num_channels = num;
    channel = new Channel[num];
}

void Mixer::set_start(int chn, unsigned int val)
{
    if (chn >= num_channels) {
        return;
    }
}

void Mixer::set_volume(int chn, int val)
{
    if (chn >= num_channels) {
        return;
    }
}

void Mixer::set_pan(int chn, int val)
{
    if (chn >= num_channels) {
        return;
    }
}

void Mixer::set_voicepos(int chn, float val)
{
    if (chn >= num_channels) {
        return;
    }
}

void Mixer::set_period(int chn, float val)
{
    if (chn >= num_channels) {
        return;
    }
}

void Mixer::enable_filter(bool val)
{
}
