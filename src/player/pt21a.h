#ifndef HAZE_PLAYER_PT21A_H_
#define HAZE_PLAYER_PT21A_H_

#include <cstdint>
#include "player/amiga_player.h"


struct Protracker2 : public FormatPlayer {
    Protracker2() : FormatPlayer(
        "pt2",
        "Protracker V2.1A playroutine + fixes",
        "A player based on the Protracker V2.1A replayer + V2.3D fixes",
        "Claudio Matsuoka",
        { "m.k." }
    ) {}

    haze::Player *new_player(void *, uint32_t, int) override;
};

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


class PT21A_Player : public AmigaPlayer {

    PT21A_ChannelData mt_chantemp[4];
    uint32_t mt_SampleStarts[31];
    //uint32_t mt_SongDataPtr;

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

    // Protracker methods
    void mt_music();
    void mt_NoNewAllChannels();
    void mt_GetNewNote();
    void mt_PlayVoice(const int, const int);
    void mt_SetPeriod(const int);
    void mt_NextPosition();
    void mt_NoNewPosYet();
    void mt_CheckEfx(const int);
    void mt_PerNop(const int);
    void mt_Arpeggio(const int);
    void mt_FinePortaUp(const int);
    void mt_PortaUp(const int);
    void mt_FinePortaDown(const int);
    void mt_PortaDown(const int);
    void mt_SetTonePorta(const int);
    void mt_TonePortamento(const int);
    void mt_TonePortNoChange(const int);
    void mt_Vibrato(const int);
    void mt_Vibrato2(const int);
    void mt_TonePlusVolSlide(const int);
    void mt_VibratoPlusVolSlide(const int);
    void mt_Tremolo(const int);
    void mt_SampleOffset(const int);
    void mt_VolumeSlide(const int);
    void mt_VolSlideUp(const int);
    void mt_VolSlideDown(const int);
    void mt_PositionJump(const int);
    void mt_VolumeChange(const int);
    void mt_PatternBreak(const int);
    void mt_SetSpeed(const int);
    void mt_CheckMoreEfx(const int);
    void mt_E_Commands(const int);
    void mt_FilterOnOff(const int);
    void mt_SetGlissControl(const int);
    void mt_SetVibratoControl(const int);
    void mt_SetFinetune(const int);
    void mt_JumpLoop(const int);
    void mt_SetTremoloControl(const int);
    void mt_RetrigNote(const int);
    void mt_DoRetrig(const int);
    void mt_VolumeFineUp(const int);
    void mt_VolumeFineDown(const int);
    void mt_NoteCut(const int);
    void mt_NoteDelay(const int);
    void mt_PatternDelay(const int);
    void mt_FunkIt(const int);
    void mt_UpdateFunk(const int);

public:
    PT21A_Player(void *, uint32_t, int);
    ~PT21A_Player();
    void start() override;
    void play() override;
    void reset() override;
    void frame_info(haze::FrameInfo&) override;
};


#endif  // HAZE_PLAYER_PT21A_H_

