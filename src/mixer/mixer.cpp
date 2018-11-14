#include "mixer/mixer.h"
#include <cstring>
#include <memory>
#include <mixer/channel.h>


constexpr int DefaultRate = 44100;


Mixer::Mixer(int num, int sr) :
    srate(sr),
    num_channels(num)//,
    //channel(num, Channel(NearestNeighborInterpolatorType))
{
    for (int i = 0; i < num; i++) {
        Channel *c = new Channel(i, NearestNeighborInterpolatorType);
        channel.push_back(c);
    }
}

Mixer::~Mixer()
{
    for (int i = channel.size() - 1; i >= 0; i--) {
        delete channel[i];
    }
}

void Mixer::add_sample(void *buf, uint32_t size)
{
    Sample s(buf, size);
    sample.push_back(s);
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
    channel[chn]->set_volume(val);
}

void Mixer::set_pan(int chn, int val)
{
    if (chn >= num_channels) {
        return;
    }
    channel[chn]->set_pan(val);
}

void Mixer::set_voicepos(int chn, double val)
{
    if (chn >= num_channels) {
        return;
    }
}

void Mixer::set_period(int chn, double val)
{
    if (chn >= num_channels) {
        return;
    }
    channel[chn]->set_period(val);
}

void Mixer::enable_filter(bool val)
{
}

void Mixer::mix(int16_t *buf, int size)
{
    std::memset(buf, 0, size * sizeof(uint16_t));

    for (int16_t *b = buf; b < (buf + size); b++) {
        for (auto c : channel) {
            *b += c->sample();
        }
    }
}
