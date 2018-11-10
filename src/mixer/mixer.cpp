#include <mixer/mixer.h>
#include <memory>

constexpr int DefaultRate = 44100;

Mixer::Mixer(int num)
{
    //channel = std::make_shared<Channel[]>(num);
    channel = new Channel[num];
}
