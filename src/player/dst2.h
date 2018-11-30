#ifndef HAZE_PLAYER_ST_H_
#define HAZE_PLAYER_ST_H_

#include <cstdint>
#include "player/player_registry.h"
#include "player/amiga_player.h"


struct DOC_Soundtracker_2 : public FormatPlayer {
    DOC_Soundtracker_2() : FormatPlayer(
        "st",
        "D.O.C SoundTracker V2.0",
        "Port of the D.O.C. SoundTracker V2.0 playroutine by Unknown of D.O.C",
        "Claudio Matsuoka",
        { "st" }
    ) {}

    haze::Player *new_player(void *, uint32_t, int) override;
};

struct ST_AudTemp {
    uint16_t n_0_note;
    uint8_t  n_2_cmd;
    uint8_t  n_3_cmdlo;
    uint32_t n_4_samplestart;
    uint16_t n_8_length;
    uint32_t n_10_loopstart;
    uint16_t n_14_replen;
    int16_t  n_16_period;
    uint8_t  n_18_volume;
    int16_t  n_22_last_note;
};

class ST_Player : public AmigaPlayer {

    uint8_t    mt_speed;
    uint8_t    mt_partnote;
    uint8_t    mt_partnrplay;
    uint8_t    mt_counter;
    uint16_t   mt_maxpart;
    uint16_t   mt_dmacon;
    bool       mt_status;
    uint32_t   mt_sample1[31];
    ST_AudTemp mt_audtemp[4];

    // Soundtracker methods
    void mt_music();
    void mt_portup(const int);
    void mt_portdown(const int);
    void mt_arpegrt(const int);
    void mt_rout2();
    void mt_playit(const int, const int);
    void mt_posjmp(const int);
    void mt_setvol(const int);
    void mt_break();
    void mt_setfil(const int);
    void mt_setspeed(const int);

public:
    ST_Player(void *, uint32_t, int);
    ~ST_Player();
    void start() override;
    void play() override;
    int length() override;
    void frame_info(haze::FrameInfo&) override;
    State save_state() override;
    void restore_state(State const&) override;
};


#endif  // HAZE_PLAYER_ST_H_
