#ifndef HAZE_PLAYER_PT21A_H_
#define HAZE_PLAYER_PT21A_H_

#include <cstdint>
#include "mixer/mixer.h"
#include "player/player.h"


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


class PT21A_Player : public haze::Player {

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

    // Protracker implementation
    void mt_music();
    void mt_NoNewAllChannels();
    void mt_GetNewNote();
    void mt_PlayVoice(int, int);
    void mt_SetPeriod(int);
    void mt_NextPosition();
    void mt_NoNewPosYet();
    void mt_CheckEfx(int);
    void mt_PerNop(int);
    void mt_Arpeggio(int);
    void mt_FinePortaUp(int);
    void mt_PortaUp(int);
    void mt_FinePortaDown(int);
    void mt_PortaDown(int);
    void mt_SetTonePorta(int);
    void mt_TonePortamento(int);
    void mt_TonePortNoChange(int);
    void mt_Vibrato(int);
    void mt_Vibrato2(int);
    void mt_TonePlusVolSlide(int);
    void mt_VibratoPlusVolSlide(int);
    void mt_Tremolo(int);
    void mt_SampleOffset(int);
    void mt_VolumeSlide(int);
    void mt_VolSlideUp(int, int val);
    void mt_VolSlideDown(int);
    void mt_PositionJump(int);
    void mt_VolumeChange(int);
    void mt_PatternBreak(int);
    void mt_SetSpeed(int);
    void mt_CheckMoreEfx(int);
    void mt_E_Commands(int);
    void mt_FilterOnOff(int);
    void mt_SetGlissControl(int);
    void mt_SetVibratoControl(int);
    void mt_SetFinetune(int);
    void mt_JumpLoop(int);
    void mt_SetTremoloControl(int);
    void mt_RetrigNote(int);
    void mt_VolumeFineUp(int);
    void mt_VolumeFineDown(int);
    void mt_NoteCut(int);
    void mt_NoteDelay(int);
    void mt_PatternDelay(int);
    void mt_FunkIt(int);
    void mt_UpdateFunk(int);
public:
    PT21A_Player(void *, uint32_t, int);
    virtual ~PT21A_Player();
    void start() override;
    void play() override;
    void reset() override;
    void frame_info(haze::FrameInfo&) override;

};


#endif  // HAZE_PLAYER_PT21A_H_

