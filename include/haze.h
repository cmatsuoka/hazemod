#ifndef HAZE_H_
#define HAZE_H_

#include <string>
#include <vector>
#include <memory>

#if defined(_WIN32) && !defined(__CYGWIN__)
#  if defined(BUILDING_STATIC)
#    define HAZE_EXPORT
#  elif defined(BUILDING_DLL)
#    define HAZE_EXPORT __declspec(dllexport)
#  else
#    define HAZE_EXPORT __declspec(dllimport)
#  endif
#elif (defined(__GNUC__) || defined(__clang__) || defined(__HP_cc)) && defined(HAZE_SYM_EXPORT)
#  define HAZE_EXPORT __attribute__((visibility ("default")))
#elif defined(__SUNPRO_CC) && defined(HAZE_SYM_EXPORT)
#  define HAZE_EXPORT __global
#elif defined(EMSCRIPTEN)
#  define HAZE_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#  define HAZE_EXPORT
#endif


namespace haze HAZE_EXPORT {

constexpr int MaxSequences = 16;

struct ModuleInfo {
    std::string title;        // Module title
    std::string format_id;    // Module format identifier
    std::string description;  // Module format description
    std::string creator;      // Program used to create this module (usually a tracker)
    std::string player_id;    // Primary player for this format
    int num_channels;         // Number of channels used in the module
    int length;               // Module length in patterns
    int num_instruments;      // Number of instruments
    std::vector<std::string> instruments;  // Instrument names
};

struct PlayerInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::vector<std::string> formats;
};

struct FormatInfo {
    std::string id;
    std::string name;
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
    HazePlayer(void *, int, std::string const&, std::string const& = "", int = 44100);
    ~HazePlayer();
    HazePlayer& player_info(PlayerInfo &);
    HazePlayer& frame_info(FrameInfo &);
    bool fill(int16_t *, int);
    bool fill(float *, int);
    void set_position(int);
};


bool probe(void *, int, ModuleInfo&);
std::vector<PlayerInfo> list_players();
std::vector<FormatInfo> list_formats();

}  // namespace haze

#endif  // HAZE_H_
