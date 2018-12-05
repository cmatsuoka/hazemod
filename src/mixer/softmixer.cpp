#include "mixer/softmixer.h"
#include <cstring>
#include <memory>
#include <algorithm>
#include "util/debug.h"

constexpr int32_t Lim16_lo = -32768;
constexpr int32_t Lim16_hi = 32767;


SoftMixer::SoftMixer(int num, int sr, Options opt) :
    Mixer(num, sr)
{
    auto iname = opt.get("interpolator", "");

    InterpolatorType itpt = LinearInterpolatorType;
    if (iname == "nearest") {
        itpt = NearestInterpolatorType;
    } else if (iname == "spline") {
        itpt = SplineInterpolatorType;
    }

    for (int i = 0; i < num; i++) {
        Voice *c = new Voice(i, itpt);
        voice.push_back(c);
    }
}

SoftMixer::~SoftMixer()
{
    for (int i = voice.size() - 1; i >= 0; i--) {
        delete voice[i];
    }
}

void SoftMixer::add_sample(void *buf, uint32_t size, double rate, uint32_t flags)
{
    Sample s(buf, size, rate, flags);
    sample.push_back(s);
}

void SoftMixer::reset()
{
    for (auto& v : voice) {
        v->reset();
    }
}

void SoftMixer::set_sample(int chn, int val)
{
    //Debug("%d, %d", chn, val);
    if (chn >= num_voices || val <= 0 || size_t(val) > sample.size()) {
        return;
    }
    voice[chn]->set_sample(sample[val - 1]);
    voice[chn]->set_start(0);
    voice[chn]->set_end(sample[val - 1].size());
}

void SoftMixer::set_start(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_start(val);
}

void SoftMixer::set_end(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_end(val);
}

void SoftMixer::set_loop_start(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_loop_start(val);
}

void SoftMixer::set_loop_end(int chn, uint32_t val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_loop_end(val);
}

void SoftMixer::enable_loop(int chn, bool val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->enable_loop(val);
}

void SoftMixer::set_volume(int chn, int val)
{
    //Debug("%d, %d", chn, val);
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_volume(val);
}

void SoftMixer::set_pan(int chn, int val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_pan(val);
}

void SoftMixer::set_voicepos(int chn, double val)
{
    if (chn >= num_voices) {
        return;
    }
    voice[chn]->set_voicepos(val);
}

void SoftMixer::set_period(int chn, double val)
{
    if (chn >= num_voices) {
        return;
    }

    Voice *v = voice[chn];
    if (val < 0.1) {
        v->set_enable(false);
    } else {
        v->set_enable(true);
        v->set_step(C4Period * C4PalRate * v->sample().rate() / srate / val);
    }
}

void SoftMixer::mix(int16_t *buf, int size)
{
    int16_t *b = buf;
    while (b < buf + size) {
        int32_t l = 0, r = 0;
        for (auto v : voice) {
            if (!v->enable()) {
                continue;
            }
            int32_t val = (v->get() * v->volume());
            r += (val * (0x80 - v->pan())) >> 6;
            l += (val * (0x80 + v->pan())) >> 6;
        }
        *b++ = std::clamp(r >> 12, Lim16_lo, Lim16_hi);
        *b++ = std::clamp(l >> 12, Lim16_lo, Lim16_hi);
    }
}

void SoftMixer::mix(float *buf, int size)
{
    float *b = buf;
    while (b < buf + size) {
        int32_t l = 0, r = 0;
        for (auto v : voice) {
            int32_t val = v->get() * v->volume();
            r += val * (0x80 - v->pan());
            l += val * (0x80 + v->pan());
        }
        //*b++ = std::clamp(r / (1 << 31), -1.0f, 1.0f);
        //*b++ = std::clamp(l / (1 << 31), -1.0f, 1.0f);
    }
}
