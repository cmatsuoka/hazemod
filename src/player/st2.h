#ifndef HAZE_PLAYER_ST2PLAY_ST2_H_
#define HAZE_PLAYER_ST2PLAY_ST2_H_

#include <cstdint>
#include "player/player_registry.h"
#include "player/pc_player.h"
#include "util/databuffer.h"

extern "C" {
#include "player/st2play/st2play.h"
#include "player/st2play/stmload.h"
}


struct St2Player : public FormatPlayer {
    St2Player() : FormatPlayer(
        "st2",
        "st2play",
        "A port of Scream Tracker 2.xx's replayer",
        R"(Sergei "x0r" Kolzun)",
        { "stm" }
    ) {}

    haze::Player *new_player(void *, uint32_t, std::string const&, int) override;
};

class ST2_Player : public PCPlayer {
    st2_context_t context;
    int length_;
    int srate_;
    double tempo_factor_;

    void load(st2_context_t *, DataBuffer const&);
public:
    ST2_Player(void *, uint32_t, int);
    ~ST2_Player();
    void start() override;
    void play() override;
    int length() override;
    void frame_info(haze::FrameInfo&) override;
    State save_state() override;
    void restore_state(State const&) override;

};


#endif  // HAZE_PLAYER_ST2PLAY_ST2_H_
