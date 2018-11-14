#include "mixer/channel.h"

constexpr int MaxChannelVolume = 1024;


Channel::Channel() :
    pos_(0.0),
    period_(0.0),
    volbase_(MaxChannelVolume),
    volume_(MaxChannelVolume),
    pan_(0),
    mute_(false),
    loop_(false),
    finish_(false),
    loop_start_(0),
    loop_end_(0),
    end_(0),
    smp_(empty_sample),
    itp_(nullptr)
{
};
