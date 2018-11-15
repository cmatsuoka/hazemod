#include "mixer/mixer.h"
#include <cstring>
#include <memory>
#include <algorithm>
#include <mixer/voice.h>


Mixer::Mixer(int num, int sr) :
    srate(sr),
    num_voices(num)//,
    //voice(num, Voice(NearestNeighborInterpolatorType))
{
    for (int i = 0; i < num; i++) {
        Voice *c = new Voice(i, NearestNeighborInterpolatorType);
        voice.push_back(c);
    }
}

Mixer::~Mixer()
{
    for (int i = voice.size() - 1; i >= 0; i--) {
        delete voice[i];
    }
}

void Mixer::add_sample(void *buf, uint32_t size)
{
    Sample s(buf, size);
    sample.push_back(s);
}

void Mixer::set_sample(int chn, int val)
{
    if (chn >= num_voices || val >= sample.size()) {
        return;
    }
    voice[chn]->set_smp(sample[val]);
}

void Mixer::set_start(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_start(val);
}

void Mixer::set_end(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_end(val);
}

void Mixer::set_loop_start(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_loop_start(val);
}

void Mixer::set_loop_end(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_loop_end(val);
}

void Mixer::enable_loop(int chn, bool val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->enable_loop(val);
}

void Mixer::set_volume(int chn, int val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_volume(val);
}

void Mixer::set_pan(int chn, int val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_pan(val);
}

void Mixer::set_voicepos(int chn, double val)
{
    if (chn >= num_voices) {
        return;
    }
}

void Mixer::set_period(int chn, double val)
{
    if (chn >= num_voices) {
        return;
    }
    Voice *ch = voice[chn];
    ch->set_step(C4Period * C4PalRate * ch->smp().rate() / srate / val);
}

void Mixer::enable_filter(bool val)
{
}

void Mixer::mix(int16_t *buf, int size)
{
    for (int16_t *b = buf; b < (buf + size); b++) {
        uint32_t val = 0;
        for (auto v : voice) {
            val += v->do_sample();
        }
        *b = std::clamp(val >> 12, Lim16_lo, Lim16_hi);
    }
}

void Mixer::mix(float *buf, int size)
{
    for (float *b = buf; b < (buf + size); b++) {
        uint32_t val = 0;
        for (auto v : voice) {
            val += v->do_sample();
        }
        *b = val / (1 << 31);
    }
}
