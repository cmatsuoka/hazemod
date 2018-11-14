#include <haze.h>
#include "mixer/mixer.h"
#include "player/player.h"
#include "player/pt21a.h"
#include "format/mod.h"


namespace haze {

bool probe(void *buf, int size, ModuleInfo& mi)
{
    ModFormat fmt;
    return fmt.probe(buf, size, mi);
}

HazePlayer::HazePlayer(void *buf, int size, std::string const& id) :
    data(buf),
    data_size(size),
    player_id(id)
{
    Mixer *m = new Mixer(4, 44100);
    player = new PT21A_Player(m, buf, size);
}

HazePlayer& HazePlayer::fill(int16_t *buf, int size)
{
    player->fill(buf, size);
    return *this;
}

}  // namespace haze
