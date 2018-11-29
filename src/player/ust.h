#ifndef HAZE_PLAYER_UST_H_
#define HAZE_PLAYER_UST_H_

#include <cstdint>
#include "player/player_registry.h"
#include "player/amiga_player.h"


struct UltimateSoundtracker : public FormatPlayer {
    UltimateSoundtracker() : FormatPlayer(
        "ust",
        "Ultimate Soundtracker V27",
        "Port of the Ultimate Soundtracker replayer version 27 (29.03.1988)",
        "Claudio Matsuoka",
        { "st" }
    ) {}

    haze::Player *new_player(void *, uint32_t, int) override;
};

//------------------------------------------------
// used varibles
//------------------------------------------------
//       datachx - structure     (22 bytes)
//
//       00.w    current note
//       02.b    sound-number
//       03.b    effect-number
//       04.l    soundstart
//       08.w    soundlenght in words
//       10.l    repeatstart
//       14.w    repeatlength
//       16.w    last saved note
//       18.w    volume
//       20.w    volume trigger (note on dynamic)
//       22.w    dma-bit
//------------------------------------------------

struct UST_datachnx {
    int16_t  n_0_note;
    uint8_t  n_2_sound_number;
    uint8_t  n_3_effect_number;
    uint32_t n_4_soundstart;
    uint16_t n_8_soundlength;
    uint32_t n_10_repeatstart;
    uint16_t n_14_repeatlength;
    int16_t  n_16_last_saved_note;
    int16_t  n_18_volume;
    int16_t  n_20_volume_trigger;
    //uint16_t n_22_dma_bit;
};

class UST_Player : public AmigaPlayer {

    UST_datachnx datachn[4];
    uint32_t     pointers[15];
    //uint32_t     lev6save;
    uint16_t     trkpos;
    uint8_t      patpos;
    uint16_t     numpat;
    uint16_t     enbits;
    uint16_t     timpos;

    // Ultimate Soundtracker methods
    void replay_muzak();
    void ceff5(const int);
    void arpreggiato(const int);
    void pitchbend(const int);
    void replaystep();
    void chanelhandler(const int, const int);

public:
    UST_Player(void *, uint32_t, int);
    ~UST_Player();
    void start() override;
    void play() override;
    int length() override;
    void frame_info(haze::FrameInfo&) override;
    State save_state() override;
    void restore_state(State const&) override;
};


#endif  // HAZE_PLAYER_UST_H_
