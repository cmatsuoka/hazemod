#ifndef HAZE_H_
#define HAZE_H_

#include <string>
#include <memory>

namespace haze {

struct ModuleInfo {
    std::string title;
};

struct PlayerInfo {
    int pos;
    int row;
    int frame;
    int song;
    int speed;
    float tempo;
    float time;
};

class Player_ {
    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void info(PlayerInfo&) = 0;
};

class HazePlayer {
    void *data;
    int data_size;
    std::string player_id;
    Player_* player;
public:
    HazePlayer(void *, int, std::string const& = "");
    HazePlayer& info(ModuleInfo&);
    template<typename T> HazePlayer& fill(T *, int, bool = false);
};


}  // namespace haze

#endif  // HAZE_H_
