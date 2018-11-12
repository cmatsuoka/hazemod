#include <haze.h>

#include "mixer/mixer.h"
#include "player/player.h"
#include "player/pt21a.h"


namespace haze {


HazePlayer::HazePlayer(void *buf, int size, std::string const& id) :
    data(buf),
    data_size(size),
    player_id(id)
{
    Mixer m(4);
    player = new PT21A_Player(m, buf, size);
}

HazePlayer& HazePlayer::info(ModuleInfo& info)
{
    info.title = "fixme";
    return *this;
}

template<typename T>
HazePlayer& HazePlayer::fill(T *buf, int size, bool loop)
{
    return *this;
}

}  // namespace haze
