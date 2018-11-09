#include "mixer.h"
#include <memory>

constexpr int DefaultRate = 44100;

template<typename T>
Mixer<T>::Mixer(int num)
{
    channel = std::make_shared<Channel[]>(num);
}
