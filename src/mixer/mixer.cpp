#include "mixer/mixer.h"
#include <cstring>
#include <memory>
#include <algorithm>
#include <mixer/voice.h>


Mixer::Mixer(int num, int sr, InterpolatorType itpt) :
    srate(sr),
    num_voices(num)//,
{
    for (int i = 0; i < num; i++) {
        Voice *c = new Voice(i, itpt);
        voice.push_back(c);
    }
}

Mixer::~Mixer()
{
    for (int i = voice.size() - 1; i >= 0; i--) {
        delete voice[i];
    }
}

void Mixer::add_sample(void *buf, uint32_t size, double rate, uint32_t flags)
{
    Sample s(buf, size, rate, flags);
    sample.push_back(s);
}

void Mixer::set_sample(int chn, int val)
{
    if (chn >= num_voices || val >= sample.size()) {
        return;
    }
    voice[chn]->set_sample(sample[val]);
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
    Voice *v = voice[chn];
    v->set_step(C4Period * C4PalRate * v->sample().rate() / srate / val);
}

void Mixer::enable_filter(bool val)
{
}

void Mixer::mix(int16_t *buf, int size)
{
    int16_t *b = buf;
    while (b < buf + size) {
        int32_t l = 0, r = 0;
        for (auto v : voice) {
            int32_t val = v->get() * v->volume();
            r += val * (0x80 - v->pan());
            l += val * (0x80 + v->pan());
        }
        *b++ = std::clamp(l >> 18, Lim16_lo, Lim16_hi);
        *b++ = std::clamp(r >> 18, Lim16_lo, Lim16_hi);
    }
}

void Mixer::mix(float *buf, int size)
{
    float *b = buf;
    while (b < buf + size) {
        int32_t l = 0, r = 0;
        for (auto v : voice) {
            int32_t val = v->get() * v->volume();
            r += val * (0x80 - v->pan());
            l += val * (0x80 + v->pan());
        }
        //*b++ = std::clamp(l / (1 << 31), -1.0f, 1.0f);
        //*b++ = std::clamp(r / (1 << 31), -1.0f, 1.0f);
    }
}
