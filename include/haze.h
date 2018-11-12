#ifndef HAZE_H_
#define HAZE_H_

#include <string>
#include <memory>

namespace haze {

struct ModuleInfo {
    std::string title;
};


class HazePlayer {
    void *data;
    int data_size;
    std::string player_id;
    class Player;
    std::unique_ptr<Player> *p;
public:
    HazePlayer(void *, int, std::string const& = "");
    HazePlayer& info(ModuleInfo&);
};


}  // namespace haze

#endif  // HAZE_H_
