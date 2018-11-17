#ifndef HAZE_PLAYER_NT11_H_
#define HAZE_PLAYER_NT11_H_

#include <cstdint>
#include "player/player.h"


struct Noisetracker : public FormatPlayer {
    Noisetracker() : FormatPlayer(
        "nt",
        "NoiseReplayV1.1 + fixes",
        "A player based on the on Noisetracker V1.1 replayer",
        "Claudio Matsuoka",
        { "m.k." }
    ) {}

    haze::Player *new_player(void *, uint32_t, int) override;
};

struct NT11_ChannelData {
    uint16_t n_0_note;
    uint8_t  n_2_cmd;
    uint8_t  n_3_cmdlo;
    uint32_t n_4_samplestart;
    uint16_t n_8_length;
    uint32_t n_a_loopstart;
    uint16_t n_e_replen;
    int16_t  n_10_period;
    uint8_t  n_12_volume;
    //uint16_t n_14_dma_control;
    bool n_16_portdir;
    uint8_t  n_17_toneportspd;
    int16_t  n_18_wantperiod;
    uint8_t  n_1a_vibrato;
    uint8_t  n_1b_vibpos;
};


class NT11_Player : public haze::Player {

    uint8_t mt_speed;
    uint8_t mt_songpos;
    uint8_t mt_pattpos;
    uint8_t mt_counter;
    bool    mt_break;

    uint32_t mt_samplestarts[31];
    NT11_ChannelData mt_voice[4];

    // Noisetracker methods
    void mt_music();
    void mt_arpeggio(int);
    void mt_getnew();
    void mt_playvoice(int, int);
    void mt_setperiod(int);
    void mt_nex();
    void mt_setmyport(int);
    void mt_myport(int);
    void mt_vib(int);
    void mt_checkcom(int);
    void mt_volslide(int);
    void mt_portup(int);
    void mt_portdown(int);
    void mt_checkcom2(int);
    void mt_setfilt(int);
    void mt_pattbreak();
    void mt_posjmp(int);
    void mt_setvol(int);
    void mt_setspeed(int);

public:
    NT11_Player(void *, uint32_t, int);
    ~NT11_Player();
    void start() override;
    void play() override;
    void reset() override;
    void frame_info(haze::FrameInfo&) override;

};


#endif  // HAZE_PLAYER_NT11_H_
