#ifndef HAZE_H_
#define HAZE_H_

#include <string>

namespace haze {

struct ModuleInfo {
    std::string title;
};


class HazePlayer {
    void *data;
    int data_size;
    std::string player_id;
public:
    HazePlayer(void *, int, std::string const& = "");
    HazePlayer& info(ModuleInfo&);
};


}  // namespace haze

#endif  // HAZE_H_
