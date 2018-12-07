#ifndef HAZE_PLAYER_FT2PLAY_FT2_H_
#define HAZE_PLAYER_FT2PLAY_FT2_H_

#include <cstdint>
#include "player/player_registry.h"
#include "player/pc_player.h"
#include "util/databuffer.h"
#include "player/ft2play/ft2play.h"


struct Ft2Player : public FormatPlayer {
    Ft2Player() : FormatPlayer(
        "ft2",
        "ft2play 0.82",
        "A port of Fasttracker 2's replayer",
        R"(Olav "8bitbubsy" Sørensen)",
        { "xm", "m.k.", "xchn" }
    ) {}

    haze::Player *new_player(void *, uint32_t, std::string const&, int) override;
};

class FT2_Player : public PCPlayer {
    Ft2Play ft2play;
    int length_;

public:
    FT2_Player(void *, uint32_t, std::string const&, int);
    ~FT2_Player();
    void start() override;
    void play() override;
    int length() override { return length_; }
    void frame_info(haze::FrameInfo&) override;
    State save_state() override;
    void restore_state(State const&) override;
};


#endif  // HAZE_PLAYER_FT2PLAY_FT2_H_
