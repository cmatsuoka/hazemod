#include <haze.h>

namespace haze {

HazePlayer::HazePlayer(void *buf, int size, std::string const& id) :
    data(buf),
    data_size(size),
    player_id(id)
{
 
}

HazePlayer& HazePlayer::info(ModuleInfo& info)
{
    info.title = "fixme";
    return *this;
}


}  // namespace haze
