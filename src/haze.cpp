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
    player_id(id)
{
    player_ = new PT21A_Player(buf, size, 44100);
}

HazePlayer::~HazePlayer()
{
    delete player_;
}

HazePlayer& HazePlayer::fill(int16_t *buf, int size)
{
    player_->fill(buf, size);
    return *this;
}

}  // namespace haze
