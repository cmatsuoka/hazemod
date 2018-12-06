#ifndef HAZE_PLAYER_HMN_H_
#define HAZE_PLAYER_HMN_H_

#include <cstdint>
#include "player/player_registry.h"
#include "player/amiga_player.h"


struct HisMastersNoise : public FormatPlayer {
    HisMastersNoise() : FormatPlayer(
        "hmn",
        "His Master's Noise replayer",
        "Jag vill helst ha en get i julklapp",
        "Claudio Matsuoka",
        { "fest", "m.k." }
    ) {}

    haze::Player *new_player(void *, uint32_t, std::string const&, int) override;
};

struct HMN_ChannelData {
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
    bool     n_16_portdir;
    uint8_t  n_17_toneportspd;
    int16_t  n_18_wantperiod;
    uint8_t  n_1a_vibrato;
    uint8_t  n_1b_vibpos;
    bool     n_1c_prog_on;

    // progdata
    uint8_t  n_8_dataloopstart;
    uint8_t  n_9_dataloopend;
    uint8_t  n_13_volume;
    uint8_t  n_1d_prog_datacou;
    int8_t   n_1e_finetune;
};


class HMN_Player : public AmigaPlayer {

    //uint16_t L658_inst;
    uint8_t  L695_counter;
    uint8_t  L642_speed;
    uint8_t  L693_songpos;
    uint8_t  L692_pattpos;
    uint16_t L701_dmacon;
    //uint16_t L49_2_vol[4];
    uint32_t L698_samplestarts[31];
    bool     L681_pattbrk;

    HMN_ChannelData voice[4];

    // HisMastersNoise methods
    void L505_2_music();
    void L505_8_arpeggio(const int);
    void L505_F_getnew();
    void L505_J_playvoice(const int, const int);
    void ProgHandler(const int);
    void setmyport(const int);
    void myport(const int);
    void myslide(const int);
    void vib(const int);
    void vibrato(const int);
    void megaarp(const int);
    void percalc(const int, int16_t);
    void L577_2_checkcom(const int);
    void L577_3_volslide(const int);
    void vibvolslide(const int);
    void myportvolslide(const int);
    void L577_7_portup(const int);
    void L577_9_portdown(const int);
    void L577_2_checkcom2(const int);
    void L577_H_setfilt(const int);
    void L577_i_pattbreak();
    void L577_J_mt_posjmp(const int);
    void L577_K_setvol(const int);
    void L577_M_setspeed(const int);

public:
    HMN_Player(void *, uint32_t, int);
    ~HMN_Player();
    void start() override;
    void play() override;
    int length() override;
    void frame_info(haze::FrameInfo&) override;
    State save_state() override;
    void restore_state(State const&) override;

};


#endif  // HAZE_PLAYER_HMN_H_
