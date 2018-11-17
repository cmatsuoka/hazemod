#include <haze.h>
#include "mixer/mixer.h"
#include "player/player.h"
#include "player/pt21a.h"
#include "player/nt11.h"
#include "format/mod.h"


namespace haze {

bool probe(void *buf, int size, ModuleInfo& mi)
{
    ModFormat fmt;
    return fmt.probe(buf, size, mi);
}

HazePlayer::HazePlayer(void *buf, int size)
{
    player_ = new NT11_Player(buf, size, 44100);
    player_->start();
}

HazePlayer::~HazePlayer()
{
    delete player_;
}

HazePlayer& HazePlayer::player_info(PlayerInfo &pi)
{
    player_->player_info(pi);
    return *this;
}

HazePlayer& HazePlayer::frame_info(FrameInfo &fi)
{
    player_->frame_info(fi);
    return *this;
}

HazePlayer& HazePlayer::fill(int16_t *buf, int size)
{
    player_->fill(buf, size);
    return *this;
}

}  // namespace haze
