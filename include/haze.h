#ifndef HAZE_H_
#define HAZE_H_

#include <string>


namespace haze {

struct ProbeInfo {
    std::string id;
    std::string title;
};


class HazePlayer {
    void *data;
    uint32_t data_size;
    std::string player_id;
public:
    HazePlayer(void *, uint32_t, std::string const& = "");
};


}  // namespace haze

#endif  // HAZE_H_
