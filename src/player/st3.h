#ifndef HAZE_PLAYER_ST3PLAY_ST3_H_
#define HAZE_PLAYER_ST3PLAY_ST3_H_

#include <cstdint>
#include "player/player_registry.h"
#include "player/pc_player.h"
#include "util/databuffer.h"
#include "player/st3play/st3play.h"


struct St3Player : public FormatPlayer {
    St3Player() : FormatPlayer(
        "st3",
        "st3play 0.88",
        "A port of Scream Tracker 3.21 replayer",
        R"(Olav "8bitbubsy" Sørensen)",
        { "s3m", "m.k.", "xchn" }
    ) {}

    haze::Player *new_player(void *, uint32_t, std::string const&, int) override;
};

class ST3_Player : public PCPlayer {
    st3play::St3Play st3play;
    int length_;

public:
    ST3_Player(void *, uint32_t, std::string const&, int);
    ~ST3_Player();
    void start() override;
    void play() override;
    int length() override { return length_; }
    void frame_info(haze::FrameInfo&) override;
    State save_state() override;
    void restore_state(State const&) override;
};


#endif  // HAZE_PLAYER_ST3PLAY_ST3_H_
