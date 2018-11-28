#include <haze.h>
#include "mixer/mixer.h"
#include "player/player.h"
#include "player/player_registry.h"
#include "format/format_registry.h"


namespace haze {

bool probe(void *buf, int size, ModuleInfo& mi)
{
    FormatRegistry reg;
    for (auto fmt : reg) {
        if (fmt->probe(buf, size, mi)) {
            return true;
        }
    }
    return false;
}

HazePlayer::HazePlayer(void *buf, int size, std::string const& player_id, std::string const& format_id)
{
    PlayerRegistry reg;

    if (reg.count(player_id) == 0) {
        throw std::runtime_error(R"(player ")" + player_id + R"(" not registered)");
    }

    FormatPlayer *fp = reg[player_id];
    if (!format_id.empty() && !fp->accepts(format_id)) {
        throw std::runtime_error(R"(format ")" + format_id +
            R"(" not accepted by player ")" + player_id + R"(")");
    }
    player_info_ = fp->info();
    player_ = fp->new_player(buf, size, 44100);
    player_->start();
    player_->scan();
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

bool HazePlayer::fill(int16_t *buf, int size)
{
    return player_->fill(buf, size);
}

void HazePlayer::set_position(const int pos)
{
    player_->set_position(pos);
}

std::vector<PlayerInfo> list_players()
{
    PlayerRegistry reg;
    std::vector<PlayerInfo> list;
    for (auto kv : reg) {
        list.push_back(kv.second->info());
    }
    return list;
}

std::vector<FormatInfo> list_formats()
{
    FormatRegistry reg;
    std::vector<FormatInfo> list;
    for (auto fmt : reg) {
        list.push_back(fmt->info());
    }
    return list;
}

}  // namespace haze
