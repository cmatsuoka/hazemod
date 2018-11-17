#include <haze.h>
#include "mixer/mixer.h"
#include "player/player.h"
#include "player/pt21a.h"
#include "player/nt11.h"
#include "format/format.h"


namespace haze {

bool probe(void *buf, int size, ModuleInfo& mi)
{
    auto reg = FormatRegistry();
    for (auto fmt : reg.list()) {
        if (fmt->probe(buf, size, mi)) {
            return true;
        }
    }
    return false;
}

HazePlayer::HazePlayer(void *buf, int size, std::string const& player_id, std::string const& format_id)
{
    PlayerRegistry reg;

    FormatPlayer *fp = reg.get(player_id);
    if (!format_id.empty() && !fp->accepts(format_id)) {
        throw std::runtime_error(R"(format ")" + format_id +
            R"(" not accepted by player ")" + player_id + R"(")");
    }
    player_info_ = fp->info();
    player_ = fp->new_player(buf, size, 44100);
    player_->start();
}

HazePlayer::~HazePlayer()
{
    delete player_;
}

HazePlayer& HazePlayer::player_info(PlayerInfo &pi)
{
    pi.id = player_info_.id;
    pi.name = player_info_.name;
    pi.description = player_info_.description;
    pi.author = player_info_.author;
    pi.formats = player_info_.formats;
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
