#ifndef HAZE_PLAYER_ST3PLAY_ST3PLAY_H
#define HAZE_PLAYER_ST3PLAY_ST3PLAY_H

#include <cstdint>
#include "mixer/softmixer.h"
#include "util/databuffer.h"

enum {
    SOUNDCARD_GUS = 0, // default
    SOUNDCARD_SB  = 1
};

struct St3Instrument {
    int8_t *data, vol;
    uint8_t type, flags;
    uint16_t c2spd;
    uint32_t length, loopbeg, looplen;
};

struct chn_t {
    int8_t aorgvol, avol, apanpos;
    uint8_t channelnum, amixtype, achannelused, aglis, atremor, atreon, atrigcnt, anotecutcnt, anotedelaycnt;
    uint8_t avibtretype, note, ins, vol, cmd, info, lastins, lastnote, alastnfo, alasteff, alasteff1;
    int16_t avibcnt, asldspd, aspd, aorgspd;
    uint16_t astartoffset, astartoffset00, ac2spd;
};

struct voice_t {
    const int8_t *m_base8;
    const int16_t *m_base16;
    uint8_t m_loopflag;
    uint16_t m_vol_l, m_vol_r;
    uint32_t m_pos, m_end, m_looplen, m_posfrac, m_speed;
    void (*m_mixfunc)(void *, int32_t); /* function pointer to mix routine */
};

/* GLOBAL DATA */
class St3Play {
    char songname[28 + 1];
    int8_t **smpPtrs, volslidetype, patterndelay, patloopcount, musicPaused;
    int8_t lastachannelused, oldstvib, fastvolslide, amigalimits, stereomode;
    uint8_t order[256], chnsettings[32], *patdata[100], *np_patseg;
    uint8_t musicmax, soundcardtype, breakpat, startrow, musiccount, interpolationFlag;
    int16_t jmptoord, np_ord, np_row, np_pat, np_patoff, patloopstart, jumptorow, globalvol, aspdmin, aspdmax;
    uint16_t useglobalvol, patmusicrand, ordNum, insNum, patNum;
    int32_t mastermul, mastervol = 256, samplesLeft, soundBufferSize, *mixBufferL, *mixBufferR;
    uint32_t samplesPerTick, audioRate, sampleCounter;
    chn_t chn[32];
    voice_t voice[32];

    St3Instrument ins[100];
    //mixRoutine mixRoutineTable[8];

    int tempo_;
    SoftMixer *mixer_;
    bool inside_loop_;

    void getlastnfo(chn_t *);
    void setspeed(uint8_t);
    void settempo(uint8_t);
    void setspd(chn_t *ch);
    void setglobalvol(int8_t);
    void setvol(chn_t *, uint8_t);
    int16_t stnote2herz(uint8_t);
    int16_t scalec2spd(chn_t *, int16_t);
    int16_t roundspd(chn_t *, int16_t);
    int16_t neworder();
    int8_t getnote1();
    void doamiga(uint8_t);
    void donewnote(uint8_t, int8_t);
    void donotes();
    void docmd1();
    void docmd2();
    void dorow();

    void voiceSetSource(uint8_t, const int /*const int8_t **/, int32_t, int32_t, int32_t, uint8_t, uint8_t);
    void voiceSetSamplePosition(uint8_t, uint16_t);
    void voiceSetVolume(uint8_t, uint16_t, uint8_t);

    friend class ST3_Player;

public:
    St3Play();
    ~St3Play();
    void s_ret(chn_t *);
    void s_setgliss(chn_t *);
    void s_setfinetune(chn_t *);
    void s_setvibwave(chn_t *);
    void s_settrewave(chn_t *);
    void s_setpanpos(chn_t *);
    void s_stereocntr(chn_t *);
    void s_patloop(chn_t *);
    void s_notecut(chn_t *);
    void s_notecutb(chn_t *);
    void s_notedelay(chn_t *);
    void s_notedelayb(chn_t *);
    void s_patterdelay(chn_t *);
    void s_setspeed(chn_t *);
    void s_jmpto(chn_t *);
    void s_break(chn_t *);
    void s_volslide(chn_t *);
    void s_slidedown(chn_t *);
    void s_slideup(chn_t *);
    void s_toneslide(chn_t *);
    void s_vibrato(chn_t *);
    void s_tremor(chn_t *);
    void s_arp(chn_t *);
    void s_vibvol(chn_t *);
    void s_tonevol(chn_t *);
    void s_retrig(chn_t *);
    void s_tremolo(chn_t *);
    void s_scommand1(chn_t *);
    void s_scommand2(chn_t *);
    void s_settempo(chn_t *);
    void s_finevibrato(chn_t *);
    void s_setgvol(chn_t *);

    void load_s3m(DataBuffer const&, const int, SoftMixer *);
};


#endif  // HAZE_PLAYER_ST3PLAY_ST3PLAY_H

