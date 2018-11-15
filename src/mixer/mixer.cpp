#include "mixer/mixer.h"
#include <cstring>
#include <memory>
#include <mixer/channel.h>


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

void Mixer::set_sample(int chn, int val)
{
    if (chn >= num_channels || val >= sample.size()) {
        return;
    }
    channel[chn]->set_smp(sample[val]);
}

void Mixer::set_start(int chn, uint32_t val)
{
    if (chn >= num_channels) {
        return;
    }
    channel[chn]->set_start(val);
}

void Mixer::set_end(int chn, uint32_t val)
{
    if (chn >= num_channels) {
        return;
    }
    channel[chn]->set_end(val);
}

void Mixer::set_loop_start(int chn, uint32_t val)
{
    if (chn >= num_channels) {
        return;
    }
    channel[chn]->set_loop_start(val);
}

void Mixer::set_loop_end(int chn, uint32_t val)
{
    if (chn >= num_channels) {
        return;
    }
    channel[chn]->set_loop_end(val);
}

void Mixer::enable_loop(int chn, bool val)
{
    if (chn >= num_channels) {
        return;
    }
    channel[chn]->enable_loop(val);
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
printf("set_period: %d: %f\n", chn, val);
    if (chn >= num_channels) {
        return;
    }
    Channel *ch = channel[chn];
    ch->set_step(C4Period * C4PalRate * ch->smp().rate() / srate / val);
}

void Mixer::enable_filter(bool val)
{
}

void Mixer::mix(int16_t *buf, int size)
{
    for (int16_t *b = buf; b < (buf + size); b++) {
        uint32_t v = 0;
        for (auto c : channel) {
            v += c->do_sample();
        }
        *b = v >> 16;
    }
}

void Mixer::mix(float *buf, int size)
{
    for (float *b = buf; b < (buf + size); b++) {
        uint32_t v = 0;
        for (auto c : channel) {
            v += c->do_sample();
        }
        *b = v / (1 << 31);
    }
}
