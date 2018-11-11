#include <haze.h>

namespace haze {

HazePlayer::HazePlayer(void *buf, uint32_t size, std::string const& id) :
    data(buf),
    data_size(size),
    player_id(id)
{
 
}

}  // namespace haze
