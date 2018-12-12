#ifndef HAZE_PLAYER_FT2PLAY_FT2PLAY_H_
#define HAZE_PLAYER_FT2PLAY_FT2PLAY_H_

#include <cstdint>
#include <cstddef>
#include <cstring>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define MAX_NOTES ((12 * 10 * 16) + 16)
#define MAX_VOICES 32
#define INSTR_SIZE           232
#define INSTR_HEADER_SIZE    263
#define MIX_BUF_SAMPLES 4096


#if 0
#define SWAP16(value) \
( \
    (((uint16_t)((value) & 0x00FF)) << 8) | \
    (((uint16_t)((value) & 0xFF00)) >> 8)   \
)
#endif

/* fast 32-bit -> 16-bit clamp */
#define CLAMP16(i) \
    if ((int16_t)(i) != i) \
        i = 0x7FFF ^ (i >> 31); \

enum
{
    IS_Vol      = 1,
    IS_Period   = 2,
    IS_NyTon    = 4,
    IS_Pan      = 8,
    IS_QuickVol = 16
};

/* *** STRUCTS *** */
typedef struct songMODInstrHeaderTyp_t
{
    char name[22];
    uint16_t len;
    uint8_t fine, vol;
    uint16_t repS, repL;
}
songMODInstrHeaderTyp;

typedef struct songMOD31HeaderTyp_t
{
    char name[20];
    songMODInstrHeaderTyp instr[31];
    uint8_t len, repS, songTab[128];
    char sig[4];
}
songMOD31HeaderTyp;

typedef struct songMOD15HeaderTyp_t
{
    char name[20];
    songMODInstrHeaderTyp instr[15];
    uint8_t len, repS, songTab[128];
}
songMOD15HeaderTyp;

typedef struct songHeaderTyp_t
{
    char sig[17], name[21], progName[20];
    uint16_t ver;
    int32_t headerSize;
    uint16_t len, repS, antChn, antPtn, antInstrs, flags, defTempo, defSpeed;
    uint8_t songTab[256];
}
songHeaderTyp;

typedef struct sampleHeaderTyp_t
{
    int32_t len, repS, repL;
    uint8_t vol;
    int8_t fine;
    uint8_t typ, pan;
    int8_t relTon;
    uint8_t skrap;
    char name[22];
}
sampleHeaderTyp;

typedef struct instrHeaderTyp_t
{
    int32_t instrSize;
    char name[22];
    uint8_t typ;
    uint16_t antSamp;
    int32_t sampleSize;
    uint8_t ta[96];
    int16_t envVP[12][2], envPP[12][2];
    uint8_t envVPAnt, envPPAnt, envVSust, envVRepS, envVRepE, envPSust, envPRepS;
    uint8_t envPRepE, envVTyp, envPTyp, vibTyp, vibSweep, vibDepth, vibRate;
    uint16_t fadeOut;
    uint8_t midiOn, midiChannel;
    int16_t midiProgram, midiBend;
    int8_t mute;
    uint8_t reserved[15];
    sampleHeaderTyp samp[32];
}
instrHeaderTyp;

typedef struct patternHeaderTyp_t
{
    int32_t patternHeaderSize;
    uint8_t typ;
    uint16_t pattLen, dataLen;
}
patternHeaderTyp;

typedef struct songTyp_t
{
    uint8_t antChn, pattDelTime, pattDelTime2, pBreakFlag;
    uint8_t pBreakPos, posJumpFlag, songTab[256], isModified;
    int16_t songPos, pattNr, pattPos, pattLen;
    uint16_t len, repS, speed, tempo, globVol, timer, ver;
    char name[21];
} songTyp;

typedef struct sampleTyp_t  /* DO NOT TOUCH!!! (order and datatypes are important) */
{
    int32_t len, repS, repL;
    uint8_t vol;
    int8_t fine;
    uint8_t typ, pan;
    int8_t relTon;
    uint8_t reserved;
    char name[22];
    int8_t *pek;
} sampleTyp;

typedef struct instrTyp_t  /* DO NOT TOUCH!!! (order and datatypes are important) */
{
    uint8_t ta[96];
    int16_t envVP[12][2], envPP[12][2];
    uint8_t envVPAnt, envPPAnt;
    uint8_t envVSust, envVRepS, envVRepE;
    uint8_t envPSust, envPRepS, envPRepE;
    uint8_t envVTyp, envPTyp;
    uint8_t vibTyp, vibSweep, vibDepth, vibRate;
    uint16_t fadeOut;
    uint8_t midiOn, midiChannel;
    int16_t midiProgram, midiBend;
    uint8_t mute, reserved[15];
    int16_t antSamp;
    sampleTyp samp[16];
} instrTyp;

typedef struct stmTyp_t
{
    volatile uint8_t status, tmpStatus;
    int8_t relTonNr, fineTune;
    uint8_t sampleNr, stOff, effTyp, eff, smpOffset, tremorSave, tremorPos;
    uint8_t globVolSlideSpeed, panningSlideSpeed, mute, waveCtrl, portaDir;
    uint8_t glissFunk, vibPos, tremPos, vibSpeed, vibDepth, tremSpeed, tremDepth;
    uint8_t pattPos, loopCnt, volSlideSpeed, fVolSlideUpSpeed, fVolSlideDownSpeed;
    uint8_t fPortaUpSpeed, fPortaDownSpeed, ePortaUpSpeed, ePortaDownSpeed;
    uint8_t portaUpSpeed, portaDownSpeed, retrigSpeed, retrigCnt, retrigVol;
    uint8_t volKolVol, tonNr, envPPos, eVibPos, envVPos, realVol, oldVol, outVol;
    uint8_t oldPan, outPan, finalPan, envSustainActive;
    int16_t realPeriod, envVIPValue, envPIPValue;
    uint16_t finalVol, outPeriod, finalPeriod, instrNr, tonTyp, wantPeriod, portaSpeed;
    uint16_t envVCnt, envVAmp, envPCnt, envPAmp, eVibAmp, eVibSweep;
    uint16_t fadeOutAmp, fadeOutSpeed;
    int32_t smpStartPos; /* 9xx */
    sampleTyp *smpPtr;
    instrTyp *instrPtr;
} stmTyp;

typedef struct tonTyp_t
{
    uint8_t ton, instr, vol, effTyp, eff;
} tonTyp;

typedef void (*mixRoutine)(void *, int32_t);

typedef struct
{
    const int8_t *sampleData8;
    const int16_t *sampleData16;
    uint8_t backwards, SVol, SPan, isFadeOutVoice;
    int32_t SLVol1, SRVol1, SLVol2, SRVol2, SLVolIP, SRVolIP, SVolIPLen, SPos, SLen, SRepS, SRepL;
    uint32_t SPosDec, SFrq;
    void (*mixRoutine)(void *, int32_t); /* function pointer to mix routine */
} voice_t;

typedef struct
{
    uint8_t *_ptr, *_base;
    int32_t _eof;
    uint32_t _cnt, _bufsiz;

    uint16_t read32l() {
        if (_cnt < 4) {
            _ptr = _base + _bufsiz;
            _cnt = 0;
            _eof = true;
            return 0;
        }
        uint16_t ret = _ptr[0] + (_ptr[1] << 8) + (_ptr[2] << 16) + (_ptr[3] << 24);
        _cnt -= 4;
        _ptr += 4;
        return ret;
    }

    uint16_t read16l() {
        if (_cnt < 2) {
            _ptr = _base + _bufsiz;
            _cnt = 0;
            _eof = true;
            return 0;
        }
        uint16_t ret = _ptr[0] + (_ptr[1] << 8);
        _cnt -= 2;
        _ptr += 2;
        return ret;
    }

    uint16_t read16b() {
        if (_cnt < 2) {
            _ptr = _base + _bufsiz;
            _cnt = 0;
            _eof = true;
            return 0;
        }
        uint16_t ret = (_ptr[0] << 8) + _ptr[1];
        _cnt -= 2;
        _ptr += 2;
        return ret;
    }

    uint16_t read8() {
        if (_cnt < 1) {
            _ptr = _base + _bufsiz;
            _cnt = 0;
            _eof = true;
            return 0;
        }
        uint8_t ret = *_ptr;
        _cnt--;
        _ptr++;
        return ret;
    }

    uint16_t read8i() {
        if (_cnt < 1) {
            _ptr = _base + _bufsiz;
            _cnt = 0;
            _eof = true;
            return 0;
        }
        int8_t ret = *_ptr;
        _cnt--;
        _ptr++;
        return ret;
    }

    size_t read(void *buffer, size_t wrcnt) {
        int32_t pcnt;

        if (_eof) {
            return 0;
        }

        pcnt = ((uint32_t)(_cnt) > wrcnt) ? wrcnt : _cnt;
        memcpy(buffer, _ptr, pcnt);

        _cnt -= pcnt;
        _ptr += pcnt;

        if (_cnt <= 0) {
            _ptr = _base + _bufsiz;
            _cnt = 0;
            _eof = true;
        }

        return pcnt;
    }
} MEM;


class Ft2Play {
    /* PRE-ZEROED VARIABLES */
    int8_t *vibSineTab, linearFrqTab, interpolationFlag, volumeRampingFlag;
    bool moduleLoaded, musicPaused, songPlaying;
    int16_t *linearPeriods, *amigaPeriods, *note2Period;
    uint16_t pattLens[256];
    int32_t masterVol, amp, quickVolSizeVal, pmpLeft, soundBufferSize, *mixBufferL, *mixBufferR;
    uint32_t *logTab, speedVal, sampleCounter, realReplayRate, frequenceDivFactor, frequenceMulFactor;
    songTyp song;
    tonTyp *patt[256];
    stmTyp stm[MAX_VOICES];
    tonTyp nilPatternLine;
    instrTyp *instr[1 + 128];
    voice_t voice[MAX_VOICES * 2];
    mixRoutine mixRoutineTable[24];

    /* FUNCTION DECLARATIONS */
    MEM *mopen(const uint8_t *, uint32_t);
    void mclose(MEM **);
    size_t mread(void *, size_t, size_t, MEM *);
    int32_t meof(MEM *);
    void mseek(MEM *, int32_t, int32_t);
    int8_t openMixer(uint32_t);
    void closeMixer();

    void setSpeed(uint16_t);
    void retrigVolume(stmTyp *);
    void retrigEnvelopeVibrato(stmTyp *);
    void keyOff(stmTyp *);
    uint32_t getFrequenceValue(uint16_t);
    void startTone(uint8_t, uint8_t, uint8_t, stmTyp *);
    void multiRetrig(stmTyp *);
    void checkMoreEffects(stmTyp *);
    void checkEffects(stmTyp *);
    void fixTonePorta(stmTyp *, tonTyp *, uint8_t);
    void getNewNote(stmTyp *, tonTyp *);
    void fixaEnvelopeVibrato(stmTyp *);
    int16_t relocateTon(int16_t, int8_t, stmTyp *);
    void tonePorta(stmTyp *);
    void volume(stmTyp *) /* actually volume slide */;
    void vibrato2(stmTyp *);
    void vibrato(stmTyp *);
    void doEffects(stmTyp *);
    void voiceUpdateVolumes(uint8_t, uint8_t);
    void voiceSetSource(uint8_t, const int8_t *, int32_t, int32_t, int32_t, int32_t, int8_t, int8_t, int32_t);
    void mix_SaveIPVolumes() /* for volume ramping */;
    void mix_UpdateChannelVolPanFrq();
    void mix8bNoLoop(voice_t *, uint32_t);
    void mix8bLoop(voice_t *, uint32_t);
    void mix8bBidiLoop(voice_t *, uint32_t);
    void mix8bNoLoopLerp(voice_t *, uint32_t);
    void mix8bLoopLerp(voice_t *, uint32_t);
    void mix8bBidiLoopLerp(voice_t *, uint32_t);
    void mix8bRampNoLoop(voice_t *, uint32_t);
    void mix8bRampLoop(voice_t *, uint32_t);
    void mix8bRampBidiLoop(voice_t *, uint32_t);
    void mix8bRampNoLoopLerp(voice_t *, uint32_t);
    void mix8bRampLoopLerp(voice_t *, uint32_t);
    void mix8bRampBidiLoopLerp(voice_t *, uint32_t);
    void mix16bNoLoop(voice_t *, uint32_t);
    void mix16bLoop(voice_t *, uint32_t);
    void mix16bBidiLoop(voice_t *, uint32_t);
    void mix16bNoLoopLerp(voice_t *, uint32_t);
    void mix16bLoopLerp(voice_t *, uint32_t);
    void mix16bBidiLoopLerp(voice_t *, uint32_t);
    void mix16bRampNoLoop(voice_t *, uint32_t);
    void mix16bRampLoop(voice_t *, uint32_t);
    void mix16bRampBidiLoop(voice_t *, uint32_t);
    void mix16bRampNoLoopLerp(voice_t *, uint32_t);
    void mix16bRampLoopLerp(voice_t *, uint32_t);
    void mix16bRampBidiLoopLerp(voice_t *, uint32_t);
    void noNewAllChannels();
    void getNextPos();
    void mainPlayer() /* periodically called from mixer */;
    void stopVoice(uint8_t);
    void stopVoices();
    void setPos(int16_t, int16_t);
    void freeInstr(uint16_t);
    void freeAllInstr();
    void freeAllPatterns();
    int8_t allocateInstr(uint16_t);
    void delta2Samp(int8_t *, uint32_t, uint8_t);
    int8_t loadInstrHeader(MEM *, uint16_t);
    void fixSample(sampleTyp *);
    void checkSampleRepeat(sampleTyp *);
    int8_t loadInstrSample(MEM *, uint16_t);
    void unpackPatt(uint8_t *, uint16_t, uint16_t, uint8_t);
    int8_t patternEmpty(uint16_t);
    int8_t loadPatterns(MEM *, uint16_t);
    int8_t loadMusicMOD(MEM *, uint32_t);
    int8_t loadMusic(const uint8_t *, uint32_t);
    void mixAudio(int16_t *, int32_t);

    bool inside_loop_;

    friend class FT2_Player;

public:
    int8_t ft2play_PlaySong(const uint8_t *, uint32_t, bool, bool, uint32_t);
    void ft2play_Close();
    void ft2play_PauseSong(bool);         // true/false
    void ft2play_TogglePause();
    void ft2play_SetMasterVol(uint16_t);  // 0..256
    void ft2play_SetAmp(uint8_t);         // 1..32
    void ft2play_SetInterpolation(bool);  // true/false
    void ft2play_SetVolumeRamping(bool);  // true/false
    char *ft2play_GetSongName();          // max 20 chars (21 with '\0'), string is in code page 437
    void ft2play_FillAudioBuffer(int16_t *, int32_t);
    uint32_t ft2play_GetMixerTicks();     // returns the amount of milliseconds of mixed audio (not realtime)
};


#endif  // HAZE_PLAYER_FT2PLAY_FT2PLAY_H_
