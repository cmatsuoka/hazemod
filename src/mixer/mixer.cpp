#include "mixer/mixer.h"
#include <cstring>
#include <memory>
#include <algorithm>
#include <mixer/voice.h>

constexpr int32_t Lim16_lo = -32768;
constexpr int32_t Lim16_hi = 32767;


Mixer::Mixer(int num, int sr, InterpolatorType itpt) :
    srate(sr),
    num_voices(num)
{
}

Mixer::~Mixer()
{
}

