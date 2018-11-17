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

struct FrameInfo {
    int pos;
    int row;
    int frame;
    int song;
    int speed;
    float tempo;
    float time;
};

class Player;

class HazePlayer {
    Player *player_;
public:
    HazePlayer(void *, int);
    ~HazePlayer();
    HazePlayer& frame_info(FrameInfo &);
    HazePlayer& fill(int16_t *, int);
    HazePlayer& fill(float *, int);
};


bool probe(void *, int, ModuleInfo&);

}  // namespace haze

#endif  // HAZE_H_
