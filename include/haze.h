#ifndef HAZE_H_
#define HAZE_H_

#include <string>
#include <memory>

namespace haze {

struct ModuleInfo {
    std::string title;        // The module title
    std::string format_id;    // The module format identifier
    std::string description;  // The module format description
    std::string creator;      // The program used to create this module (usually a tracker)
    std::string player;       // The primary player for this format
    int channels;             // The number of channels used in the module
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
public:
    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void info(PlayerInfo&) = 0;
    virtual void fill(int16_t *, int) = 0;
};

class HazePlayer {
    void *data;
    int data_size;
    std::string player_id;
    Player_ *player;
public:
    HazePlayer(void *, int, std::string const& = "");
    HazePlayer& fill(int16_t *, int);
};


bool probe(void *, int, ModuleInfo&);

}  // namespace haze

#endif  // HAZE_H_
