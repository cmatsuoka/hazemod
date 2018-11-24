#ifndef HAZE_H_
#define HAZE_H_

#include <string>
#include <vector>
#include <memory>


namespace haze {

constexpr int MaxSequences = 16;


struct ModuleInfo {
    std::string title;        // The module title
    std::string format_id;    // The module format identifier
    std::string description;  // The module format description
    std::string creator;      // The program used to create this module (usually a tracker)
    std::string player_id;    // The primary player for this format
    int channels;             // The number of channels used in the module
    int length;
};

struct PlayerInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::vector<std::string> formats;
};

struct FrameInfo {
    int pos;         // Current position
    int row;         // Current row in pattern
    int num_rows;    // Number of rows in current pattern
    int frame;       // Current frame
    int song;        // Current song
    int speed;       // Current speed
    int loop_count;  // Loop counter
    int tempo;       // Current tempo
    int time;        // Current replay time
    int total_time;  // Total replay time
};

class Player;

class HazePlayer {
    Player *player_;
    PlayerInfo player_info_;
public:
    HazePlayer(void *, int, std::string const&, std::string const& = "");
    ~HazePlayer();
    HazePlayer& player_info(PlayerInfo &);
    HazePlayer& frame_info(FrameInfo &);
    HazePlayer& fill(int16_t *, int);
    HazePlayer& fill(float *, int);
};


bool probe(void *, int, ModuleInfo&);

}  // namespace haze

#endif  // HAZE_H_
