#ifndef HAZE_PLAYER_FT101_H_
#define HAZE_PLAYER_FT101_H_

#include <cstdint>
#include "player/pc_player.h"


struct FastTracker: public FormatPlayer {
    FastTracker() : FormatPlayer(
        "ft",
        "FastTracker 1.01 playroutine",
        "Based on the Protracker V2.1A replayer",
        "Claudio Matsuoka",
        { "m.k.", "xchn" }
    ) {}

    haze::Player *new_player(void *, uint32_t, int) override;
};

struct FT_ChannelData {
    uint16_t n_length;
    uint16_t n_loopstart;
    uint16_t n_replen;
    uint8_t  output_volume;
    uint8_t  n_finetune;
    uint16_t output_period;
    uint8_t  n_insnum;
    uint8_t  n_wavecontrol;
    uint8_t  n_vibratopos;
    uint8_t  n_tremolopos;
    uint16_t n_command;
    uint8_t  n_offset;
    uint16_t n_period;
    uint16_t n_wantedperiod;
    uint8_t  n_toneportdirec;
    bool     n_gliss;
    uint16_t n_toneportspeed;
    uint8_t  n_vibratospeed;
    uint8_t  n_vibratodepth;
    uint8_t  n_pattpos;
    uint8_t  n_loopcount;
    uint8_t  n_tremolospeed;
    uint8_t  n_tremolodepth;
    uint8_t  n_volume;

    bool inside_loop;
};

class FT_Player : public PCPlayer {

    uint8_t  ft_speed;
    uint8_t  ft_counter;
    uint8_t  ft_song_pos;
    uint8_t  ft_pbreak_pos;
    bool     ft_pos_jump_flag;
    bool     ft_pbreak_flag;
    uint8_t  ft_patt_del_time;
    uint8_t  ft_patt_del_time_2;
    uint8_t  ft_pattern_pos;
    uint16_t ft_current_pattern;
    uint8_t  cia_tempo;
    bool     position_jump_cmd;

    int channels;
    FT_ChannelData ft_chantemp[8];

    // FastTracker methods

public:
    FT_Player(void *, uint32_t, int);
    ~FT_Player();
    void start() override;
    void play() override;
    void reset() override;
    void frame_info(haze::FrameInfo&) override;
};


#endif  // HAZE_PLAYER_FT101_H_

