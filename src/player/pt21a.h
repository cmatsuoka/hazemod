#ifndef HAZE_PLAYER_PT21A_H_
#define HAZE_PLAYER_PT21A_H_

#include <cstdint>
#include <mixer/mixer.h>
#include <player/player.h>


struct PT21A_ChannelData {
    uint16_t n_note;
    uint8_t n_cmd;
    uint8_t n_cmdlo;
    uint32_t n_start;
    uint16_t n_length;
    uint32_t n_loopstart;
    uint16_t n_replen;
    uint16_t n_period;
    uint8_t n_finetune;
    uint8_t n_volume;
    bool n_toneportdirec;
    uint8_t n_toneportspeed;
    uint16_t n_wantedperiod;
    uint8_t n_vibratocmd;
    uint8_t n_vibratopos;
    uint8_t n_tremolocmd;
    uint8_t n_tremolopos;
    uint8_t n_wavecontrol;
    uint8_t n_glissfunk;
    uint8_t n_sampleoffset;
    uint8_t n_pattpos;
    uint8_t n_loopcount;
    uint8_t n_funkoffset;
    uint32_t n_wavestart;
    uint16_t n_reallength;

    bool inside_loop;
};


class PT21A_Player : public Player {

    PT21A_ChannelData mt_chantemp[4];
    uint32_t mt_SampleStarts[31];
    uint32_t mt_SongDataPtr;

    uint8_t mt_speed;
    uint8_t mt_counter;
    uint8_t mt_SongPos;
    uint8_t mt_PBreakPos;
    bool mt_PosJumpFlag;
    bool mt_PBreakFlag;
    uint8_t mt_LowMask;
    uint8_t mt_PattDelTime;
    uint8_t mt_PattDelTime2;
    uint8_t mt_PatternPos;

    uint8_t cia_tempo;

public:
    PT21A_Player(Mixer&);
    void start() override;
};


#endif  // HAZE_PLAYER_PT21A_H_

