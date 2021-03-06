/*
** FT2PLAY v0.97 - 27th of November 2018 - https://16-bits.org
** ===========================================================
**                 - NOT BIG ENDIAN SAFE! -
**
** Very accurate C port of Fasttracker 2's replayer (.XM/.MOD/.FT),
** by Olav "8bitbubsy" Sørensen. Using the original pascal+asm source
** codes by Fredrik "Mr.H" Huss (Triton). I have permission to make this
** C port public.
**
** You need to link winmm.lib for this to compile (-lwinmm)
** Alternatively, you can change out the mixer functions at the bottom with
** your own for your OS.
**
** Example of ft2play usage:
** #include "ft2play.h"
** #include "songdata.h"
**
** ft2play_PlaySong(songData, songDataLength, true, true, 44100);
** mainLoop();
** ft2play_Close();
**
** To turn a song into an include file like in the example, you can use my win32
** bin2h tool from here: https://16-bits.org/etc/bin2h.zip
**
** Changes in v0.97:
** - Audio channel mixer optimizations
**
** Changes in v0.96:
** - Removed some unneeded logic in the audio channel mixer
**
** Changes in v0.95:
** - Turned some unneeded 64-bit calculations into 32-bit
** - Support loading .MODs with odd (1,3,5..31) channel numbers
**
** Changes in v0.94:
** - .MOD/.FT playback was completely broken, sorry!
**
** Changes in v0.93:
** - Linear interpolation is done with 16-bit fractional precision instead of 15-bit
**
** Changes in v0.92:
** - Added a function to retrieve song name
**
** Changes in v0.91:
** - Now uses much faster audio channel mixer from my FT2 clone project
** - Added support for loading .MOD/.FT files
** - WinMM mixer has been rewritten to be safe (DON'T use syscalls in callback -MSDN)
** - Some small fixes
** - Some small changes to the ft2play functions (easier to use and safer!)
*/

/* ft2play.h:

#ifndef __FT2PLAY_H
#define __FT2PLAY_H

#include <stdint.h>

int8_t ft2play_PlaySong(const uint8_t *moduleData, uint32_t dataLength, int8_t useInterpolationFlag, int8_t useVolumeRampingFlag, uint32_t audioFreq);
void ft2play_Close(void);
void ft2play_PauseSong(int8_t flag); // true/false
void ft2play_TogglePause(void);
void ft2play_SetMasterVol(uint16_t vol); // 0..256
void ft2play_SetAmp(uint8_t ampFactor); // 1..32
void ft2play_SetInterpolation(uint8_t flag); // true/false
void ft2play_SetVolumeRamping(uint8_t flag); // true/false
char *ft2play_GetSongName(void); // max 20 chars (21 with '\0'), string is in code page 437
uint32_t ft2play_GetMixerTicks(void); // returns the amount of milliseconds of mixed audio (not realtime)

#endif
*/

#define MIX_BUF_SAMPLES 4096

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define SWAP16(value) \
( \
    (((uint16_t)((value) & 0x00FF)) << 8) | \
    (((uint16_t)((value) & 0xFF00)) >> 8)   \
)

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

/* *** STRUCTS *** (remember 1-byte alignment for header/loader structs) */
#ifdef _MSC_VER
#pragma pack(push)
#pragma pack(1)
#endif
typedef struct songMODInstrHeaderTyp_t
{
    char name[22];
    uint16_t len;
    uint8_t fine, vol;
    uint16_t repS, repL;
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
songMODInstrHeaderTyp;

typedef struct songMOD31HeaderTyp_t
{
    char name[20];
    songMODInstrHeaderTyp instr[31];
    uint8_t len, repS, songTab[128];
    char sig[4];
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
songMOD31HeaderTyp;

typedef struct songMOD15HeaderTyp_t
{
    char name[20];
    songMODInstrHeaderTyp instr[15];
    uint8_t len, repS, songTab[128];
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
songMOD15HeaderTyp;

typedef struct songHeaderTyp_t
{
    char sig[17], name[21], progName[20];
    uint16_t ver;
    int32_t headerSize;
    uint16_t len, repS, antChn, antPtn, antInstrs, flags, defTempo, defSpeed;
    uint8_t songTab[256];
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
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
#ifdef __GNUC__
__attribute__ ((packed))
#endif
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
#ifdef __GNUC__
__attribute__ ((packed))
#endif
instrHeaderTyp;

typedef struct patternHeaderTyp_t
{
    int32_t patternHeaderSize;
    uint8_t typ;
    uint16_t pattLen, dataLen;
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
patternHeaderTyp;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

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
} MEM;

#define MAX_NOTES ((12 * 10 * 16) + 16)
#define MAX_VOICES 32
#define INSTR_SIZE           232
#define INSTR_HEADER_SIZE    263

/* TABLES AND VARIABLES */
static const uint32_t panningTab[257] =
{
        0, 4096, 5793, 7094, 8192, 9159,10033,10837,11585,12288,12953,13585,14189,14768,15326,15864,
    16384,16888,17378,17854,18318,18770,19212,19644,20066,20480,20886,21283,21674,22058,22435,22806,
    23170,23530,23884,24232,24576,24915,25249,25580,25905,26227,26545,26859,27170,27477,27780,28081,
    28378,28672,28963,29251,29537,29819,30099,30377,30652,30924,31194,31462,31727,31991,32252,32511,
    32768,33023,33276,33527,33776,34024,34270,34514,34756,34996,35235,35472,35708,35942,36175,36406,
    36636,36864,37091,37316,37540,37763,37985,38205,38424,38642,38858,39073,39287,39500,39712,39923,
    40132,40341,40548,40755,40960,41164,41368,41570,41771,41972,42171,42369,42567,42763,42959,43154,
    43348,43541,43733,43925,44115,44305,44494,44682,44869,45056,45242,45427,45611,45795,45977,46160,
    46341,46522,46702,46881,47059,47237,47415,47591,47767,47942,48117,48291,48465,48637,48809,48981,
    49152,49322,49492,49661,49830,49998,50166,50332,50499,50665,50830,50995,51159,51323,51486,51649,
    51811,51972,52134,52294,52454,52614,52773,52932,53090,53248,53405,53562,53719,53874,54030,54185,
    54340,54494,54647,54801,54954,55106,55258,55410,55561,55712,55862,56012,56162,56311,56459,56608,
    56756,56903,57051,57198,57344,57490,57636,57781,57926,58071,58215,58359,58503,58646,58789,58931,
    59073,59215,59357,59498,59639,59779,59919,60059,60199,60338,60477,60615,60753,60891,61029,61166,
    61303,61440,61576,61712,61848,61984,62119,62254,62388,62523,62657,62790,62924,63057,63190,63323,
    63455,63587,63719,63850,63982,64113,64243,64374,64504,64634,64763,64893,65022,65151,65279,65408,
    65536
};

static const uint16_t amigaPeriod[12 * 8] =
{
    4*1712,4*1616,4*1524,4*1440,4*1356,4*1280,4*1208,4*1140,4*1076,4*1016,4*960,4*906,
    2*1712,2*1616,2*1524,2*1440,2*1356,2*1280,2*1208,2*1140,2*1076,2*1016,2*960,2*906,
    1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,906,
    856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,
    214,202,190,180,170,160,151,143,135,127,120,113,
    107,101,95,90,85,80,75,71,67,63,60,56,
    53,50,47,45,42,40,37,35,33,31,30,28
};

static const uint16_t amigaFinePeriod[12 * 8] =
{
    907,900,894,887,881,875,868,862,856,850,844,838,
    832,826,820,814,808,802,796,791,785,779,774,768,
    762,757,752,746,741,736,730,725,720,715,709,704,
    699,694,689,684,678,675,670,665,660,655,651,646,
    640,636,632,628,623,619,614,610,604,601,597,592,
    588,584,580,575,570,567,563,559,555,551,547,543,
    538,535,532,528,524,520,516,513,508,505,502,498,
    494,491,487,484,480,477,474,470,467,463,460,457
};

static const uint8_t vibTab[32] =
{
    0,  24,  49, 74, 97,120,141,161,
    180,197,212,224,235,244,250,253,
    255,253,250,244,235,224,212,197,
    180,161,141,120, 97, 74, 49, 24
};

/* PRE-ZEROED VARIABLES */
static int8_t *vibSineTab, linearFrqTab, interpolationFlag, volumeRampingFlag;
static int8_t moduleLoaded, musicPaused, songPlaying;
static int16_t *linearPeriods, *amigaPeriods, *note2Period;
static uint16_t pattLens[256];
static int32_t masterVol, amp, quickVolSizeVal, pmpLeft, soundBufferSize, *mixBufferL, *mixBufferR;
static uint32_t *logTab, speedVal, sampleCounter, realReplayRate, frequenceDivFactor, frequenceMulFactor;
static songTyp song;
static tonTyp *patt[256];
static stmTyp stm[MAX_VOICES];
static tonTyp nilPatternLine;
static instrTyp *instr[1 + 128];
static voice_t voice[MAX_VOICES * 2];
static mixRoutine mixRoutineTable[24];

/* FUNCTION DECLARATIONS */
static MEM *mopen(const uint8_t *src, uint32_t length);
static void mclose(MEM **buf);
static size_t mread(void *buffer, size_t size, size_t count, MEM *buf);
static int32_t meof(MEM *buf);
static void mseek(MEM *buf, int32_t offset, int32_t whence);
static int8_t openMixer(uint32_t audioFreq);
static void closeMixer(void);

/* CODE START */

static void setSpeed(uint16_t bpm)
{
    if (bpm > 0)
        speedVal = ((realReplayRate + realReplayRate) + (realReplayRate >> 1)) / bpm;
}

static void retrigVolume(stmTyp *ch)
{
    ch->realVol = ch->oldVol;
    ch->outVol  = ch->oldVol;
    ch->outPan  = ch->oldPan;
    ch->status |= (IS_Vol + IS_Pan + IS_QuickVol);
}

static void retrigEnvelopeVibrato(stmTyp *ch)
{
    instrTyp *ins;

    if (!(ch->waveCtrl & 0x04)) ch->vibPos  = 0;
    if (!(ch->waveCtrl & 0x40)) ch->tremPos = 0;

    ch->retrigCnt = 0;
    ch->tremorPos = 0;

    ch->envSustainActive = true;

    ins = ch->instrPtr;

    if (ins->envVTyp & 1)
    {
        ch->envVCnt = 65535;
        ch->envVPos = 0;
    }

    if (ins->envPTyp & 1)
    {
        ch->envPCnt = 65535;
        ch->envPPos = 0;
    }

    ch->fadeOutSpeed = ins->fadeOut; /* FT2 doesn't check if fadeout is more than 4095 */
    ch->fadeOutAmp   = 32768;

    if (ins->vibDepth != 0)
    {
        ch->eVibPos = 0;

        if (ins->vibSweep != 0)
        {
            ch->eVibAmp   = 0;
            ch->eVibSweep = (ins->vibDepth << 8) / ins->vibSweep;
        }
        else
        {
            ch->eVibAmp   = ins->vibDepth << 8;
            ch->eVibSweep = 0;
        }
    }
}

static void keyOff(stmTyp *ch)
{
    instrTyp *ins;

    ch->envSustainActive = false;

    ins = ch->instrPtr;

    if (!(ins->envPTyp & 1)) /* yes, FT2 does this (!) */
    {
        if (ch->envPCnt >= ins->envPP[ch->envPPos][0])
            ch->envPCnt  = ins->envPP[ch->envPPos][0] - 1;
    }

    if (ins->envVTyp & 1)
    {
        if (ch->envVCnt >= ins->envVP[ch->envVPos][0])
            ch->envVCnt  = ins->envVP[ch->envVPos][0] - 1;
    }
    else
    {
        ch->realVol = 0;
        ch->outVol  = 0;
        ch->status |= (IS_Vol + IS_QuickVol);
    }
}

/* 100% FT2-accurate routine, do not touch! */
static uint32_t getFrequenceValue(uint16_t period)
{
    uint8_t shift;
    uint16_t index;
    uint32_t rate;

    if (period == 0)
        return (0);

    if (linearFrqTab)
    {
        index = (12 * 192 * 4) - period;
        shift = (14 - (index / 768)) & 0x1F;

        /* this converts to fast code even on x86 (imul + shrd) */
        rate = ((uint64_t)(logTab[index % 768]) * frequenceMulFactor) >> (32 - 8);
        if (shift > 0)
            rate >>= shift;
    }
    else
    {
        rate = frequenceDivFactor / period;
    }

    return (rate);
}

static void startTone(uint8_t ton, uint8_t effTyp, uint8_t eff, stmTyp *ch)
{
    uint8_t smp;
    uint16_t tmpTon;
    sampleTyp *s;
    instrTyp *ins;

    /* no idea if this EVER triggers... */
    if (ton == 97)
    {
        keyOff(ch);
        return;
    }
    /* ------------------------------------------------------------ */

    /* if we came from Rxy (retrig), we didn't check note (Ton) yet */
    if (ton == 0)
    {
        ton = ch->tonNr;
        if (ton == 0) return; /* if still no note, return. */
    }
    /* ------------------------------------------------------------ */

    ch->tonNr = ton;

    if (instr[ch->instrNr] != NULL)
        ins = instr[ch->instrNr];
    else
        ins = instr[0]; /* placeholder for invalid samples */

    ch->instrPtr = ins;

    ch->mute = ins->mute;

    if (ton > 96) /* added security check */
        ton = 96;

    smp = ins->ta[ton - 1] & 0x0F;

    s = &ins->samp[smp];
    ch->smpPtr = s;

    ch->relTonNr = s->relTon;

    ton += ch->relTonNr;
    if (ton >= (12 * 10))
        return;

    ch->oldVol = s->vol;
    ch->oldPan = s->pan;

    if ((effTyp == 0x0E) && ((eff & 0xF0) == 0x50))
        ch->fineTune = ((eff & 0x0F) * 16) - 128; /* result is now -128 .. 127 */
    else
        ch->fineTune = s->fine;

    if (ton > 0)
    {
        /* MUST use >> 3 here (sar cl,3) - safe for x86/x86_64 */
        tmpTon = ((ton - 1) * 16) + (((ch->fineTune >> 3) + 16) & 0xFF);

        if (tmpTon < MAX_NOTES) /* should never happen, but FT2 does this check */
        {
            ch->realPeriod = note2Period[tmpTon];
            ch->outPeriod  = ch->realPeriod;
        }
    }

    ch->status |= (IS_Period + IS_Vol + IS_Pan + IS_NyTon + IS_QuickVol);

    if (effTyp == 9)
    {
        if (eff)
            ch->smpOffset = ch->eff;

        ch->smpStartPos = 256 * ch->smpOffset;
    }
    else
    {
        ch->smpStartPos = 0;
    }
}

static void multiRetrig(stmTyp *ch)
{
    int8_t cmd;
    uint8_t cnt;
    int16_t vol;

    cnt = ch->retrigCnt + 1;
    if (cnt < ch->retrigSpeed)
    {
        ch->retrigCnt = cnt;
        return;
    }

    ch->retrigCnt = 0;

    vol = ch->realVol;
    cmd = ch->retrigVol;

    /* 0x00 and 0x08 are not handled, ignore them */

    if (cmd == 0x01)
    {
        vol -= 1;
        if (vol < 0) vol = 0;
    }
    else if (cmd == 0x02)
    {
        vol -= 2;
        if (vol < 0) vol = 0;
    }
    else if (cmd == 0x03)
    {
        vol -= 4;
        if (vol < 0) vol = 0;
    }
    else if (cmd == 0x04)
    {
        vol -= 8;
        if (vol < 0) vol = 0;
    }
    else if (cmd == 0x05)
    {
        vol -= 16;
        if (vol < 0) vol = 0;
    }
    else if (cmd == 0x06)
    {
        vol = (vol >> 1) + (vol >> 3) + (vol >> 4);
    }
    else if (cmd == 0x07)
    {
        vol >>= 1;
    }
    else if (cmd == 0x09)
    {
        vol += 1;
        if (vol > 64) vol = 64;
    }
    else if (cmd == 0x0A)
    {
        vol += 2;
        if (vol > 64) vol = 64;
    }
    else if (cmd == 0x0B)
    {
        vol += 4;
        if (vol > 64) vol = 64;
    }
    else if (cmd == 0x0C)
    {
        vol += 8;
        if (vol > 64) vol = 64;
    }
    else if (cmd == 0x0D)
    {
        vol += 16;
        if (vol > 64) vol = 64;
    }
    else if (cmd == 0x0E)
    {
        vol = (vol >> 1) + vol;
        if (vol > 64) vol = 64;
    }
    else if (cmd == 0x0F)
    {
        vol += vol;
        if (vol > 64) vol = 64;
    }

    ch->realVol = (uint8_t)(vol);
    ch->outVol  = ch->realVol;

    if ((ch->volKolVol >= 0x10) && (ch->volKolVol <= 0x50))
    {
        ch->outVol  = ch->volKolVol - 0x10;
        ch->realVol = ch->outVol;
    }
    else if ((ch->volKolVol >= 0xC0) && (ch->volKolVol <= 0xCF))
    {
        ch->outPan = (ch->volKolVol & 0x0F) << 4;
    }

    startTone(0, 0, 0, ch);
}

static void checkMoreEffects(stmTyp *ch)
{
    int8_t envPos, envUpdate;
    uint8_t tmpEff;
    int16_t newEnvPos;
    uint16_t i;
    instrTyp *ins;

    ins = ch->instrPtr;

    /* Bxx - position jump */
    if (ch->effTyp == 11)
    {
        song.songPos     = (int16_t)(ch->eff) - 1;
        song.pBreakPos   = 0;
        song.posJumpFlag = true;
    }

    /* Dxx - pattern break */
    else if (ch->effTyp == 13)
    {
        song.posJumpFlag = true;

        tmpEff = ((ch->eff >> 4) * 10) + (ch->eff & 0x0F);
        if (tmpEff <= 63)
            song.pBreakPos = tmpEff;
        else
            song.pBreakPos = 0;
    }

    /* Exx - E effects */
    else if (ch->effTyp == 14)
    {
        /* E1x - fine period slide up */
        if ((ch->eff & 0xF0) == 0x10)
        {
            tmpEff = ch->eff & 0x0F;
            if (!tmpEff)
                tmpEff = ch->fPortaUpSpeed;

            ch->fPortaUpSpeed = tmpEff;

            ch->realPeriod -= (tmpEff * 4);
            if (ch->realPeriod < 1) ch->realPeriod = 1;

            ch->outPeriod = ch->realPeriod;
            ch->status   |= IS_Period;
        }

        /* E2x - fine period slide down */
        else if ((ch->eff & 0xF0) == 0x20)
        {
            tmpEff = ch->eff & 0x0F;
            if (!tmpEff)
                tmpEff = ch->fPortaDownSpeed;

            ch->fPortaDownSpeed = tmpEff;

            ch->realPeriod += (tmpEff * 4);
            if (ch->realPeriod > (32000 - 1)) ch->realPeriod = 32000 - 1;

            ch->outPeriod = ch->realPeriod;
            ch->status   |= IS_Period;
        }

        /* E3x - set glissando type */
        else if ((ch->eff & 0xF0) == 0x30) ch->glissFunk = ch->eff & 0x0F;

        /* E4x - set vibrato waveform */
        else if ((ch->eff & 0xF0) == 0x40) ch->waveCtrl = (ch->waveCtrl & 0xF0) | (ch->eff & 0x0F);

        /* E5x (set finetune) is handled in StartTone() */

        /* E6x - pattern loop */
        else if ((ch->eff & 0xF0) == 0x60)
        {
            if (ch->eff == 0x60) /* E60, empty param */
            {
                ch->pattPos = song.pattPos & 0x00FF;
            }
            else
            {
                if (!ch->loopCnt)
                {
                    ch->loopCnt = ch->eff & 0x0F;

                    song.pBreakPos  = ch->pattPos;
                    song.pBreakFlag = true;
                }
                else
                {
                    if (--ch->loopCnt)
                    {
                        song.pBreakPos  = ch->pattPos;
                        song.pBreakFlag = true;
                    }
                }
            }
        }

        /* E7x - set tremolo waveform */
        else if ((ch->eff & 0xF0) == 0x70) ch->waveCtrl = ((ch->eff & 0x0F) << 4) | (ch->waveCtrl & 0x0F);

        /* E8x - set 4-bit panning (NON-FT2) */
        else if ((ch->eff & 0xF0) == 0x80)
        {
            ch->outPan  = (ch->eff & 0x0F) * 16;
            ch->status |= IS_Pan;
        }

        /* EAx - fine volume slide up */
        else if ((ch->eff & 0xF0) == 0xA0)
        {
            tmpEff = ch->eff & 0x0F;
            if (!tmpEff)
                tmpEff = ch->fVolSlideUpSpeed;

            ch->fVolSlideUpSpeed = tmpEff;

            /* unsigned clamp */
            if (ch->realVol <= (64 - tmpEff))
                ch->realVol += tmpEff;
            else
                ch->realVol = 64;

            ch->outVol = ch->realVol;
            ch->status |= IS_Vol;
        }

        /* EBx - fine volume slide down */
        else if ((ch->eff & 0xF0) == 0xB0)
        {
            tmpEff = ch->eff & 0x0F;
            if (!tmpEff)
                tmpEff = ch->fVolSlideDownSpeed;

            ch->fVolSlideDownSpeed = tmpEff;

            /* unsigned clamp */
            if (ch->realVol >= tmpEff)
                ch->realVol -= tmpEff;
            else
                ch->realVol = 0;

            ch->outVol = ch->realVol;
            ch->status |= IS_Vol;
        }

        /* ECx - note cut */
        else if ((ch->eff & 0xF0) == 0xC0)
        {
            if (ch->eff == 0xC0) /* empty param */
            {
                ch->realVol = 0;
                ch->outVol = 0;
                ch->status |= (IS_Vol + IS_QuickVol);
            }
        }

        /* EEx - pattern delay */
        else if ((ch->eff & 0xF0) == 0xE0)
        {
            if (!song.pattDelTime2)
                song.pattDelTime = (ch->eff & 0x0F) + 1;
        }
    }

    /* Fxx - set speed/tempo */
    else if (ch->effTyp == 15)
    {
        if (ch->eff >= 32)
        {
            song.speed = ch->eff;
            setSpeed(song.speed);
        }
        else
        {
            song.tempo = ch->eff;
            song.timer = ch->eff;
        }
    }

    /* Gxx - set global volume */
    else if (ch->effTyp == 16)
    {
        song.globVol = ch->eff;
        if (song.globVol > 64) song.globVol = 64;

        for (i = 0; i < song.antChn; ++i)
            stm[i].status |= IS_Vol;
    }

    /* Lxx - set vol and pan envelope position */
    else if (ch->effTyp == 21)
    {
        /* *** VOLUME ENVELOPE *** */
        if (ins->envVTyp & 1)
        {
            ch->envVCnt = ch->eff - 1;

            envPos = 0;
            envUpdate = true;
            newEnvPos = ch->eff;

            if (ins->envVPAnt > 1)
            {
                envPos++;
                for (i = 0; i < ins->envVPAnt - 1; ++i)
                {
                    if (newEnvPos < ins->envVP[envPos][0])
                    {
                        envPos--;

                        newEnvPos -= ins->envVP[envPos][0];
                        if (newEnvPos == 0)
                        {
                            envUpdate = false;
                            break;
                        }

                        if (ins->envVP[envPos + 1][0] <= ins->envVP[envPos][0])
                        {
                            envUpdate = true;
                            break;
                        }

                        ch->envVIPValue = ((ins->envVP[envPos + 1][1] - ins->envVP[envPos][1]) & 0x00FF) << 8;
                        ch->envVIPValue /= (ins->envVP[envPos + 1][0] - ins->envVP[envPos][0]);

                        ch->envVAmp = (ch->envVIPValue * (newEnvPos - 1)) + ((ins->envVP[envPos][1] & 0x00FF) << 8);

                        envPos++;

                        envUpdate = false;
                        break;
                    }

                    envPos++;
                }

                if (envUpdate) envPos--;
            }

            if (envUpdate)
            {
                ch->envVIPValue = 0;
                ch->envVAmp = (ins->envVP[envPos][1] & 0x00FF) << 8;
            }

            if (envPos >= ins->envVPAnt)
            {
                envPos = ins->envVPAnt - 1;
                if (envPos < 0)
                    envPos = 0;
            }

            ch->envVPos = envPos;
        }

        /* *** PANNING ENVELOPE *** */
        if (ins->envVTyp & 2) /* probably an FT2 bug */
        {
            ch->envPCnt = ch->eff - 1;

            envPos = 0;
            envUpdate = true;
            newEnvPos = ch->eff;

            if (ins->envPPAnt > 1)
            {
                envPos++;
                for (i = 0; i < ins->envPPAnt - 1; ++i)
                {
                    if (newEnvPos < ins->envPP[envPos][0])
                    {
                        envPos--;

                        newEnvPos -= ins->envPP[envPos][0];
                        if (newEnvPos == 0)
                        {
                            envUpdate = false;
                            break;
                        }

                        if (ins->envPP[envPos + 1][0] <= ins->envPP[envPos][0])
                        {
                            envUpdate = true;
                            break;
                        }

                        ch->envPIPValue = ((ins->envPP[envPos + 1][1] - ins->envPP[envPos][1]) & 0x00FF) << 8;
                        ch->envPIPValue /= (ins->envPP[envPos + 1][0] - ins->envPP[envPos][0]);

                        ch->envPAmp = (ch->envPIPValue * (newEnvPos - 1)) + ((ins->envPP[envPos][1] & 0x00FF) << 8);

                        envPos++;

                        envUpdate = false;
                        break;
                    }

                    envPos++;
                }

                if (envUpdate) envPos--;
            }

            if (envUpdate)
            {
                ch->envPIPValue = 0;
                ch->envPAmp = (ins->envPP[envPos][1] & 0x00FF) << 8;
            }

            if (envPos >= ins->envPPAnt)
            {
                envPos = ins->envPPAnt - 1;
                if (envPos < 0)
                    envPos = 0;
            }

            ch->envPPos = envPos;
        }
    }
}

static void checkEffects(stmTyp *ch)
{
    uint8_t tmpEff, tmpEffHi, volKol;

    /* this one is manipulated by vol column effects, then used for multiretrig (FT2 quirk) */
    volKol = ch->volKolVol;

    /* *** VOLUME COLUMN EFFECTS (TICK 0) *** */

    /* set volume */
    if ((ch->volKolVol >= 0x10) && (ch->volKolVol <= 0x50))
    {
        volKol -= 16;

        ch->outVol  = volKol;
        ch->realVol = volKol;

        ch->status |= (IS_Vol + IS_QuickVol);
    }

    /* fine volume slide down */
    else if ((ch->volKolVol & 0xF0) == 0x80)
    {
        volKol = ch->volKolVol & 0x0F;

        /* unsigned clamp */
        if (ch->realVol >= volKol)
            ch->realVol -= volKol;
        else
            ch->realVol = 0;

        ch->outVol  = ch->realVol;
        ch->status |= IS_Vol;
    }

    /* fine volume slide up */
    else if ((ch->volKolVol & 0xF0) == 0x90)
    {
        volKol = ch->volKolVol & 0x0F;

        /* unsigned clamp */
        if (ch->realVol <= (64 - volKol))
            ch->realVol += volKol;
        else
            ch->realVol = 64;

        ch->outVol  = ch->realVol;
        ch->status |= IS_Vol;
    }

    /* set vibrato speed */
    else if ((ch->volKolVol & 0xF0) == 0xA0)
    {
        volKol = (ch->volKolVol & 0x0F) << 2;
        ch->vibSpeed = volKol;
    }

    /* set panning */
    else if ((ch->volKolVol & 0xF0) == 0xC0)
    {
        volKol <<= 4;

        ch->outPan  = volKol;
        ch->status |= IS_Pan;
    }


    /* *** MAIN EFFECTS (TICK 0) *** */


    if ((ch->effTyp == 0) && (ch->eff == 0)) return;

    /* Cxx - set volume */
    if (ch->effTyp == 12)
    {
        ch->realVol = ch->eff;
        if (ch->realVol > 64) ch->realVol = 64;

        ch->outVol = ch->realVol;
        ch->status |= (IS_Vol + IS_QuickVol);

        return;
    }

    /* 8xx - set panning */
    else if (ch->effTyp == 8)
    {
        ch->outPan  = ch->eff;
        ch->status |= IS_Pan;

        return;
    }

    /* Rxy - note multi retrigger */
    else if (ch->effTyp == 27)
    {
        tmpEff = ch->eff & 0x0F;
        if (!tmpEff)
            tmpEff = ch->retrigSpeed;

        ch->retrigSpeed = tmpEff;

        tmpEffHi = ch->eff >> 4;
        if (!tmpEffHi)
            tmpEffHi = ch->retrigVol;

        ch->retrigVol = tmpEffHi;

        if (volKol == 0)
            multiRetrig(ch);

        return;
    }

    /* X1x - extra fine period slide up */
    else if ((ch->effTyp == 33) && ((ch->eff & 0xF0) == 0x10))
    {
        tmpEff = ch->eff & 0x0F;
        if (!tmpEff)
            tmpEff = ch->ePortaUpSpeed;

        ch->ePortaUpSpeed = tmpEff;

        ch->realPeriod -= tmpEff;
        if (ch->realPeriod < 1) ch->realPeriod = 1;

        ch->outPeriod = ch->realPeriod;
        ch->status |= IS_Period;

        return;
    }

    /* X2x - extra fine period slide down */
    else if ((ch->effTyp == 33) && ((ch->eff & 0xF0) == 0x20))
    {
        tmpEff = ch->eff & 0x0F;
        if (!tmpEff)
            tmpEff = ch->ePortaDownSpeed;

        ch->ePortaDownSpeed = tmpEff;

        ch->realPeriod += tmpEff;
        if (ch->realPeriod > (32000 - 1)) ch->realPeriod = 32000 - 1;

        ch->outPeriod = ch->realPeriod;
        ch->status |= IS_Period;

        return;
    }

    checkMoreEffects(ch);
}

static void fixTonePorta(stmTyp *ch, tonTyp *p, uint8_t inst)
{
    uint16_t portaTmp;

    if (p->ton)
    {
        if (p->ton == 97)
        {
            keyOff(ch);
        }
        else
        {
            /* MUST use >> 3 here (sar cl,3) - safe for x86/x86_64 */
            portaTmp = ((((p->ton - 1) + ch->relTonNr) & 0x00FF) * 16) + (((ch->fineTune >> 3) + 16) & 0x00FF);

            if (portaTmp < MAX_NOTES)
            {
                ch->wantPeriod = note2Period[portaTmp];

                     if (ch->wantPeriod == ch->realPeriod) ch->portaDir = 0;
                else if (ch->wantPeriod  > ch->realPeriod) ch->portaDir = 1;
                else                                       ch->portaDir = 2;
            }
        }
    }

    if (inst)
    {
        retrigVolume(ch);

        if (p->ton != 97)
            retrigEnvelopeVibrato(ch);
    }
}

static void getNewNote(stmTyp *ch, tonTyp *p)
{
    /* this is a mess, but it appears to be 100% FT2-correct */

    uint8_t inst, checkEfx;

    ch->volKolVol = p->vol;

    if (!ch->effTyp)
    {
        if (ch->eff)
        {
            /* we have an arpeggio running, set period back */
            ch->outPeriod = ch->realPeriod;
            ch->status   |= IS_Period;
        }
    }
    else
    {
        if ((ch->effTyp == 4) || (ch->effTyp == 6))
        {
            /* we have a vibrato running */
            if ((p->effTyp != 4) && (p->effTyp != 6))
            {
                /* but it's ending at the next (this) row, so set period back */
                ch->outPeriod = ch->realPeriod;
                ch->status   |= IS_Period;
            }
        }
    }

    ch->effTyp = p->effTyp;
    ch->eff    = p->eff;
    ch->tonTyp = (p->instr << 8) | p->ton;

    /* 'inst' var is used for later if checks... */
    inst = p->instr;
    if (inst)
    {
        if (inst <= 128)
            ch->instrNr = inst;
        else
            inst = 0;
    }

    checkEfx = true;
    if (p->effTyp == 0x0E)
    {
        if ((p->eff >= 0xD1) && (p->eff <= 0xDF))
            return; /* we have a note delay (ED1..EDF) */
        else if (p->eff == 0x90)
            checkEfx = false;
    }

    if (checkEfx)
    {
        if ((ch->volKolVol & 0xF0) == 0xF0) /* gxx */
        {
            if (ch->volKolVol & 0x0F)
                ch->portaSpeed = (ch->volKolVol & 0x0F) << 6;

            fixTonePorta(ch, p, inst);

            checkEffects(ch);

            return;
        }

        if ((p->effTyp == 3) || (p->effTyp == 5)) /* 3xx or 5xx */
        {
            if ((p->effTyp != 5) && (p->eff != 0))
                ch->portaSpeed = p->eff << 2;

            fixTonePorta(ch, p, inst);

            checkEffects(ch);

            return;
        }

        if ((p->effTyp == 0x14) && (p->eff == 0)) /* K00 (KeyOff - only handle tick 0 here) */
        {
            keyOff(ch);

            if (inst)
                retrigVolume(ch);

            checkEffects(ch);
            return;
        }

        if (!p->ton)
        {
            if (inst)
            {
                retrigVolume(ch);
                retrigEnvelopeVibrato(ch);
            }

            checkEffects(ch);
            return;
        }
    }

    if (p->ton == 97)
        keyOff(ch);
    else
        startTone(p->ton, p->effTyp, p->eff, ch);

    if (inst)
    {
        retrigVolume(ch);

        if (p->ton != 97)
            retrigEnvelopeVibrato(ch);
    }

    checkEffects(ch);
}

static void fixaEnvelopeVibrato(stmTyp *ch)
{
    int8_t envInterpolateFlag, envDidInterpolate;
    uint8_t envPos;
    int16_t autoVibVal, panTmp;
    uint16_t autoVibAmp, tmpPeriod, envVal;
    int32_t tmp32;
    instrTyp *ins;

    ins = ch->instrPtr;

    /* *** FADEOUT *** */
    if (!ch->envSustainActive)
    {
        ch->status |= IS_Vol;

        /* unsigned clamp + reset */
        if (ch->fadeOutAmp >= ch->fadeOutSpeed)
        {
            ch->fadeOutAmp -= ch->fadeOutSpeed;
        }
        else
        {
            ch->fadeOutAmp   = 0;
            ch->fadeOutSpeed = 0;
        }
    }

    if (ch->mute != 1)
    {
        /* *** VOLUME ENVELOPE *** */
        envVal = 0;
        if (ins->envVTyp & 1)
        {
            envDidInterpolate = false;
            envPos = ch->envVPos;

            if (++ch->envVCnt == ins->envVP[envPos][0])
            {
                ch->envVAmp = (ins->envVP[envPos][1] & 0x00FF) << 8;

                envPos++;
                if (ins->envVTyp & 4)
                {
                    envPos--;

                    if (envPos == ins->envVRepE)
                    {
                        if (!(ins->envVTyp & 2) || (envPos != ins->envVSust) || ch->envSustainActive)
                        {
                            envPos = ins->envVRepS;

                            ch->envVCnt =  ins->envVP[envPos][0];
                            ch->envVAmp = (ins->envVP[envPos][1] & 0x00FF) << 8;
                        }
                    }

                    envPos++;
                }

                if (envPos < ins->envVPAnt)
                {
                    envInterpolateFlag = true;
                    if ((ins->envVTyp & 2) && ch->envSustainActive)
                    {
                        if ((envPos - 1) == ins->envVSust)
                        {
                            envPos--;
                            ch->envVIPValue = 0;
                            envInterpolateFlag = false;
                        }
                    }

                    if (envInterpolateFlag)
                    {
                        ch->envVPos = envPos;

                        ch->envVIPValue = 0;
                        if (ins->envVP[envPos][0] > ins->envVP[envPos - 1][0])
                        {
                            ch->envVIPValue = ((ins->envVP[envPos][1] - ins->envVP[envPos - 1][1]) & 0x00FF) << 8;
                            ch->envVIPValue /= (ins->envVP[envPos][0] - ins->envVP[envPos - 1][0]);

                            envVal = ch->envVAmp;
                            envDidInterpolate = true;
                        }
                    }
                }
                else
                {
                    ch->envVIPValue = 0;
                }
            }

            if (!envDidInterpolate)
            {
                ch->envVAmp += ch->envVIPValue;

                envVal = ch->envVAmp;
                if ((envVal >> 8) > 0x40)
                {
                    if ((envVal >> 8) > ((0x40 + 0xC0) / 2))
                        envVal = 16384;
                    else
                        envVal = 0;

                    ch->envVIPValue = 0;
                }
            }

            envVal >>= 8;

            ch->finalVol = (song.globVol * (((envVal * ch->outVol) * ch->fadeOutAmp) >> (16 + 2))) >> 7;
            ch->status  |= IS_Vol;
        }
        else
        {
            ch->finalVol = (song.globVol * (((ch->outVol << 4) * ch->fadeOutAmp) >> 16)) >> 7;
        }

        /* FT2 doesn't clamp the volume, but that's just stupid. Let's do it! */
        if (ch->finalVol > 256)
            ch->finalVol = 256;
    }
    else
    {
        ch->finalVol = 0;
    }

    /* *** PANNING ENVELOPE *** */

    envVal = 0;
    if (ins->envPTyp & 1)
    {
        envDidInterpolate = false;
        envPos = ch->envPPos;

        if (++ch->envPCnt == ins->envPP[envPos][0])
        {
            ch->envPAmp = (ins->envPP[envPos][1] & 0x00FF) << 8;

            envPos++;
            if (ins->envPTyp & 4)
            {
                envPos--;

                if (envPos == ins->envPRepE)
                {
                    if (!(ins->envPTyp & 2) || (envPos != ins->envPSust) || ch->envSustainActive)
                    {
                        envPos = ins->envPRepS;

                        ch->envPCnt =  ins->envPP[envPos][0];
                        ch->envPAmp = (ins->envPP[envPos][1] & 0x00FF) << 8;
                    }
                }

                envPos++;
            }

            if (envPos < ins->envPPAnt)
            {
                envInterpolateFlag = true;
                if ((ins->envPTyp & 2) && ch->envSustainActive)
                {
                    if ((envPos - 1) == ins->envPSust)
                    {
                        envPos--;
                        ch->envPIPValue = 0;
                        envInterpolateFlag = false;
                    }
                }

                if (envInterpolateFlag)
                {
                    ch->envPPos = envPos;

                    ch->envPIPValue = 0;
                    if (ins->envPP[envPos][0] > ins->envPP[envPos - 1][0])
                    {
                        ch->envPIPValue  = ((ins->envPP[envPos][1] - ins->envPP[envPos - 1][1]) & 0x00FF) << 8;
                        ch->envPIPValue /=  (ins->envPP[envPos][0] - ins->envPP[envPos - 1][0]);

                        envVal = ch->envPAmp;
                        envDidInterpolate = true;
                    }
                }
            }
            else
            {
                ch->envPIPValue = 0;
            }
        }

        if (!envDidInterpolate)
        {
            ch->envPAmp += ch->envPIPValue;

            envVal = ch->envPAmp;
            if ((envVal >> 8) > 0x40)
            {
                if ((envVal >> 8) > ((0x40 + 0xC0) / 2))
                    envVal = 16384;
                else
                    envVal = 0;

                ch->envPIPValue = 0;
            }
        }

        /* panning calculated exactly like FT2 */
        panTmp = ch->outPan - 128;
        if (panTmp > 0) panTmp = 0 - panTmp;
        panTmp += 128;

        envVal -= (32 * 256);

        ch->finalPan = ch->outPan + (int8_t)((((int16_t)(envVal) * (panTmp << 3)) >> 16) & 0xFF);
        ch->status  |= IS_Pan;
    }
    else
    {
        ch->finalPan = ch->outPan;
    }

    /* *** AUTO VIBRATO *** */
    if (ins->vibDepth != 0)
    {
        if (ch->eVibSweep != 0)
        {
            autoVibAmp = ch->eVibSweep;
            if (ch->envSustainActive)
            {
                autoVibAmp += ch->eVibAmp;
                if ((autoVibAmp >> 8) > ins->vibDepth)
                {
                    autoVibAmp    = ins->vibDepth << 8;
                    ch->eVibSweep = 0;
                }

                ch->eVibAmp = autoVibAmp;
            }
        }
        else
        {
            autoVibAmp = ch->eVibAmp;
        }

        ch->eVibPos += ins->vibRate;

        /* square */
        if (ins->vibTyp == 1)
            autoVibVal = (ch->eVibPos > 127) ? 64 : -64;

        /* ramp up */
        else if (ins->vibTyp == 2)
            autoVibVal = (((ch->eVibPos >> 1) + 64) & 127) - 64;

        /* ramp down */
        else if (ins->vibTyp == 3)
            autoVibVal = (((0 - (ch->eVibPos >> 1)) + 64) & 127) - 64;

        /* sine */
        else
            autoVibVal = vibSineTab[ch->eVibPos];

        autoVibVal <<= 2;

        tmp32 = ((autoVibVal * (signed)(autoVibAmp)) >> 16) & 0x8000FFFF;
        tmpPeriod = ch->outPeriod + (int16_t)(tmp32);
        if (tmpPeriod > (32000 - 1)) tmpPeriod = 0; /* yes, FT2 zeroes it out */

        ch->finalPeriod = tmpPeriod;

        ch->status  |= IS_Period;
    }
    else
    {
        ch->finalPeriod = ch->outPeriod;
    }
}

static int16_t relocateTon(int16_t period, int8_t relativeNote, stmTyp *ch)
{
    int8_t i, fineTune;
    int16_t *periodTable;
    int32_t loPeriod, hiPeriod, tmpPeriod, tableIndex;

    fineTune    = (ch->fineTune >> 3) + 16;
    hiPeriod    = 8 * 12 * 16;
    loPeriod    = 0;
    periodTable = note2Period;

    for (i = 0; i < 8; ++i)
    {
        tmpPeriod = (((loPeriod + hiPeriod) / 2) & ~15) + fineTune;

        tableIndex = tmpPeriod - 8;
        if (tableIndex < 0) /* added security check */
            tableIndex = 0;

        if (period >= periodTable[tableIndex])
            hiPeriod = tmpPeriod - fineTune;
        else
            loPeriod = tmpPeriod - fineTune;
    }

    tmpPeriod = loPeriod + fineTune + (relativeNote * 16);
    if (tmpPeriod < 0) /* added security check */
        tmpPeriod = 0;

    if (tmpPeriod >= ((8 * 12 * 16) + 15) - 1) /* FT2 bug: stupid off-by-one edge case */
        tmpPeriod  =  (8 * 12 * 16) + 15;

    return (periodTable[tmpPeriod]);
}

static void tonePorta(stmTyp *ch)
{
    if (ch->portaDir)
    {
        if (ch->portaDir > 1)
        {
            ch->realPeriod -= ch->portaSpeed;
            if (ch->realPeriod <= ch->wantPeriod)
            {
                ch->portaDir   = 1;
                ch->realPeriod = ch->wantPeriod;
            }
        }
        else
        {
            ch->realPeriod += ch->portaSpeed;
            if (ch->realPeriod >= ch->wantPeriod)
            {
                ch->portaDir   = 1;
                ch->realPeriod = ch->wantPeriod;
            }
        }

        if (ch->glissFunk) /* semi-tone slide flag */
            ch->outPeriod = relocateTon(ch->realPeriod, 0, ch);
        else
            ch->outPeriod = ch->realPeriod;

        ch->status |= IS_Period;
    }
}

static void volume(stmTyp *ch) /* actually volume slide */
{
    uint8_t tmpEff;

    tmpEff = ch->eff;
    if (!tmpEff)
        tmpEff = ch->volSlideSpeed;

    ch->volSlideSpeed = tmpEff;

    if (!(tmpEff & 0xF0))
    {
        /* unsigned clamp */
        if (ch->realVol >= tmpEff)
            ch->realVol -= tmpEff;
        else
            ch->realVol = 0;
    }
    else
    {
        /* unsigned clamp */
        if (ch->realVol <= (64 - (tmpEff >> 4)))
            ch->realVol += (tmpEff >> 4);
        else
            ch->realVol = 64;
    }

    ch->outVol  = ch->realVol;
    ch->status |= IS_Vol;
}

static void vibrato2(stmTyp *ch)
{
    uint8_t tmpVib;

    tmpVib = (ch->vibPos / 4) & 0x1F;

    switch (ch->waveCtrl & 0x03)
    {
        /* 0: sine */
        case 0:
        {
            tmpVib = vibTab[tmpVib];
        }
        break;

        /* 1: ramp */
        case 1:
        {
            tmpVib *= 8;
            if (ch->vibPos >= 128)
                tmpVib ^= 0xFF;
        }
        break;

        /* 2/3: square */
        default:
        {
            tmpVib = 255;
        }
        break;
    }

    tmpVib = (tmpVib * ch->vibDepth) / 32;

    if (ch->vibPos >= 128)
        ch->outPeriod = ch->realPeriod - tmpVib;
    else
        ch->outPeriod = ch->realPeriod + tmpVib;

    ch->status |= IS_Period;
    ch->vibPos += ch->vibSpeed;
}

static void vibrato(stmTyp *ch)
{
    if (ch->eff)
    {
        if (ch->eff & 0x0F) ch->vibDepth = ch->eff & 0x0F;
        if (ch->eff & 0xF0) ch->vibSpeed = (ch->eff >> 4) * 4;
    }

    vibrato2(ch);
}

static void doEffects(stmTyp *ch)
{
    int8_t note;
    uint8_t tmpEff, tremorData, tremorSign, tmpTrem;
    int16_t tremVol;
    uint16_t i, tick;

    /* *** VOLUME COLUMN EFFECTS (TICKS >0) *** */

    /* volume slide down */
    if ((ch->volKolVol & 0xF0) == 0x60)
    {
        /* unsigned clamp */
        if (ch->realVol >= (ch->volKolVol & 0x0F))
            ch->realVol -= (ch->volKolVol & 0x0F);
        else
            ch->realVol = 0;

        ch->outVol  = ch->realVol;
        ch->status |= IS_Vol;
    }

    /* volume slide up */
    else if ((ch->volKolVol & 0xF0) == 0x70)
    {
        /* unsigned clamp */
        if (ch->realVol <= (64 - (ch->volKolVol & 0x0F)))
            ch->realVol += (ch->volKolVol & 0x0F);
        else
            ch->realVol = 64;

        ch->outVol  = ch->realVol;
        ch->status |= IS_Vol;
    }

    /* vibrato (+ set vibrato depth) */
    else if ((ch->volKolVol & 0xF0) == 0xB0)
    {
        if (ch->volKolVol != 0xB0)
            ch->vibDepth = ch->volKolVol & 0x0F;

        vibrato2(ch);
    }

    /* pan slide left */
    else if ((ch->volKolVol & 0xF0) == 0xD0)
    {
        /* unsigned clamp + a bug when the parameter is 0  */
        if (((ch->volKolVol & 0x0F) == 0) || (ch->outPan < (ch->volKolVol & 0x0F)))
            ch->outPan = 0;
        else
            ch->outPan -= (ch->volKolVol & 0x0F);

        ch->status |= IS_Pan;
    }

    /* pan slide right */
    else if ((ch->volKolVol & 0xF0) == 0xE0)
    {
        /* unsigned clamp */
        if (ch->outPan <= (255 - (ch->volKolVol & 0x0F)))
            ch->outPan += (ch->volKolVol & 0x0F);
        else
            ch->outPan = 255;

        ch->status |= IS_Pan;
    }

    /* tone portamento */
    else if ((ch->volKolVol & 0xF0) == 0xF0) tonePorta(ch);

    /* *** MAIN EFFECTS (TICKS >0) *** */

    if (((ch->eff == 0) && (ch->effTyp == 0)) || (ch->effTyp >= 36)) return;

    /* 0xy - Arpeggio */
    if (ch->effTyp == 0)
    {
        tick = song.timer;
        note = 0;

        /* FT2 'out of boundary' arp LUT simulation */
             if (tick  > 16) tick  = 2;
        else if (tick == 16) tick  = 0;
        else                 tick %= 3;

        /*
        ** this simulation doesn't work properly for >=128 tick arps,
        ** but you'd need to hexedit the initial speed to get >31
        */

        if (!tick)
        {
            ch->outPeriod = ch->realPeriod;
        }
        else
        {
                 if (tick == 1) note = ch->eff >> 4;
            else if (tick >  1) note = ch->eff & 0x0F;

            ch->outPeriod = relocateTon(ch->realPeriod, note, ch);
        }

        ch->status |= IS_Period;
    }

    /* 1xx - period slide up */
    else if (ch->effTyp == 1)
    {
        tmpEff = ch->eff;
        if (!tmpEff)
            tmpEff = ch->portaUpSpeed;

        ch->portaUpSpeed = tmpEff;

        ch->realPeriod -= (tmpEff * 4);
        if (ch->realPeriod < 1) ch->realPeriod = 1;

        ch->outPeriod = ch->realPeriod;
        ch->status   |= IS_Period;
    }

    /* 2xx - period slide down */
    else if (ch->effTyp == 2)
    {
        tmpEff = ch->eff;
        if (!tmpEff)
            tmpEff = ch->portaDownSpeed;

        ch->portaDownSpeed = tmpEff;

        ch->realPeriod += (tmpEff * 4);
        if (ch->realPeriod > (32000 - 1)) ch->realPeriod = 32000 - 1;

        ch->outPeriod = ch->realPeriod;
        ch->status   |= IS_Period;
    }

    /* 3xx - tone portamento */
    else if (ch->effTyp == 3) tonePorta(ch);

    /* 4xy - vibrato */
    else if (ch->effTyp == 4) vibrato(ch);

    /* 5xy - tone portamento + volume slide */
    else if (ch->effTyp == 5)
    {
        tonePorta(ch);
        volume(ch);
    }

    /* 6xy - vibrato + volume slide */
    else if (ch->effTyp == 6)
    {
        vibrato2(ch);
        volume(ch);
    }

    /* 7xy - tremolo */
    else if (ch->effTyp == 7)
    {
        tmpEff = ch->eff;
        if (tmpEff)
        {
            if (tmpEff & 0x0F) ch->tremDepth = tmpEff & 0x0F;
            if (tmpEff & 0xF0) ch->tremSpeed = (tmpEff >> 4) * 4;
        }

        tmpTrem = (ch->tremPos / 4) & 0x1F;

        switch ((ch->waveCtrl >> 4) & 3)
        {
            /* 0: sine */
            case 0:
            {
                tmpTrem = vibTab[tmpTrem];
            }
            break;

            /* 1: ramp */
            case 1:
            {
                tmpTrem *= 8;
                if (ch->vibPos >= 128) tmpTrem ^= 0xFF; /* FT2 bug, should've been TremPos */
            }
            break;

            /* 2/3: square */
            default:
            {
                tmpTrem = 255;
            }
            break;
        }

        tmpTrem = (tmpTrem * ch->tremDepth) / 64;

        if (ch->tremPos >= 128)
        {
            tremVol = ch->realVol - tmpTrem;
            if (tremVol < 0) tremVol = 0;
        }
        else
        {
            tremVol = ch->realVol + tmpTrem;
            if (tremVol > 64) tremVol = 64;
        }

        ch->outVol = tremVol & 0x00FF;

        ch->tremPos += ch->tremSpeed;

        ch->status |= IS_Vol;
    }

    /* Axy - volume slide */
    else if (ch->effTyp == 10) volume(ch); /* actually volume slide */

    /* Exy - E effects */
    else if (ch->effTyp == 14)
    {
        /* E9x - note retrigger */
        if ((ch->eff & 0xF0) == 0x90)
        {
            if (ch->eff != 0x90) /* E90 is handled in getNewNote() */
            {
                if (!((song.tempo - song.timer) % (ch->eff & 0x0F)))
                {
                    startTone(0, 0, 0, ch);
                    retrigEnvelopeVibrato(ch);
                }
            }
        }

        /* ECx - note cut */
        else if ((ch->eff & 0xF0) == 0xC0)
        {
            if (((song.tempo - song.timer) & 0x00FF) == (ch->eff & 0x0F))
            {
                ch->outVol  = 0;
                ch->realVol = 0;
                ch->status |= (IS_Vol + IS_QuickVol);
            }
        }

        /* EDx - note delay */
        else if ((ch->eff & 0xF0) == 0xD0)
        {
            if (((song.tempo - song.timer) & 0x00FF) == (ch->eff & 0x0F))
            {
                startTone(ch->tonTyp & 0x00FF, 0, 0, ch);

                if (ch->tonTyp & 0xFF00)
                    retrigVolume(ch);

                retrigEnvelopeVibrato(ch);

                if ((ch->volKolVol >= 0x10) && (ch->volKolVol <= 0x50))
                {
                    ch->outVol  = ch->volKolVol - 16;
                    ch->realVol = ch->outVol;
                }
                else if ((ch->volKolVol >= 0xC0) && (ch->volKolVol <= 0xCF))
                {
                    ch->outPan = (ch->volKolVol & 0x0F) << 4;
                }
            }
        }
    }

    /* Hxy - global volume slide */
    else if (ch->effTyp == 17)
    {
        tmpEff = ch->eff;
        if (!tmpEff)
            tmpEff = ch->globVolSlideSpeed;

        ch->globVolSlideSpeed = tmpEff;

        if (!(tmpEff & 0xF0))
        {
            /* unsigned clamp */
            if (song.globVol >= tmpEff)
                song.globVol -= tmpEff;
            else
                song.globVol = 0;
        }
        else
        {
            /* unsigned clamp */
            if (song.globVol <= (64 - (tmpEff >> 4)))
                song.globVol += (tmpEff >> 4);
            else
                song.globVol = 64;
        }

        for (i = 0; i < song.antChn; ++i)
            stm[i].status |= IS_Vol;
    }

    /* Kxx - key off */
    else if (ch->effTyp == 20)
    {
        if (((song.tempo - song.timer) & 31) == (ch->eff & 0x0F))
            keyOff(ch);
    }

    /* Pxy - panning slide */
    else if (ch->effTyp == 25)
    {
        tmpEff = ch->eff;
        if (!tmpEff)
            tmpEff = ch->panningSlideSpeed;

        ch->panningSlideSpeed = tmpEff;

        if ((tmpEff & 0xF0) == 0)
        {
            /* unsigned clamp */
            if (ch->outPan >= tmpEff)
                ch->outPan -= tmpEff;
            else
                ch->outPan = 0;
        }
        else
        {
            tmpEff >>= 4;

            /* unsigned clamp */
            if (ch->outPan <= (255 - tmpEff))
                ch->outPan += tmpEff;
            else
                ch->outPan = 255;
        }

        ch->status |= IS_Pan;
    }

    /* Rxy - multi note retrig */
    else if (ch->effTyp == 27) multiRetrig(ch);

    /* Txy - tremor */
    else if (ch->effTyp == 29)
    {
        tmpEff = ch->eff;
        if (!tmpEff)
            tmpEff = ch->tremorSave;

        ch->tremorSave = tmpEff;

        tremorSign = ch->tremorPos & 0x80;
        tremorData = ch->tremorPos & 0x7F;

        tremorData--;
        if (tremorData & 0x80)
        {
            if (tremorSign == 0x80)
            {
                tremorSign = 0x00;
                tremorData = tmpEff & 0x0F;
            }
            else
            {
                tremorSign = 0x80;
                tremorData = tmpEff >> 4;
            }
        }

        ch->tremorPos = tremorData | tremorSign;

        ch->outVol  = tremorSign ? ch->realVol : 0;
        ch->status |= (IS_Vol + IS_QuickVol);
    }
}

static void voiceUpdateVolumes(uint8_t i, uint8_t status)
{
    int32_t volL, volR;
    voice_t *v, *f;

    v = &voice[i];

    volL = v->SVol * amp;

    volR = (volL * panningTab[      v->SPan]) >> (32 - 28); /* 0..267386880 */
    volL = (volL * panningTab[256 - v->SPan]) >> (32 - 28); /* 0..267386880 */

    if (!volumeRampingFlag)
    {
        v->SLVol2 = volL;
        v->SRVol2 = volR;
    }
    else
    {
        v->SLVol1 = volL;
        v->SRVol1 = volR;

        if (status & IS_NyTon)
        {
            /* sample is about to start, ramp out/in at the same time */

            /* setup "fade out" voice (only if current voice volume>0) */
            if ((v->SLVol2 > 0) || (v->SRVol2 > 0))
            {
                f = &voice[MAX_VOICES + i];
                memcpy(f, v, sizeof (voice_t));

                f->SVolIPLen      = quickVolSizeVal;
                f->SLVolIP        = -f->SLVol2 / f->SVolIPLen;
                f->SRVolIP        = -f->SRVol2 / f->SVolIPLen;
                f->isFadeOutVoice = true;
            }

            /* make current voice fade in when it starts */
            v->SLVol2 = 0;
            v->SRVol2 = 0;
        }

        /* ramp volume changes */

        /*
        ** FT2 has two internal volume ramping lengths:
        ** IS_QuickVol: 5ms (audioFreq / 200)
        ** Normal: The duration of a tick (speedVal)
        */

        if ((volL == v->SLVol2) && (volR == v->SRVol2))
        {
            v->SVolIPLen = 0; /* there is no volume change */
        }
        else
        {
            v->SVolIPLen = (status & IS_QuickVol) ? quickVolSizeVal : speedVal;
            v->SLVolIP   = (volL - v->SLVol2) / v->SVolIPLen;
            v->SRVolIP   = (volR - v->SRVol2) / v->SVolIPLen;
        }
    }
}

static void voiceSetSource(uint8_t i, const int8_t *sampleData,
    int32_t sampleLength, int32_t sampleLoopBegin, int32_t sampleLoopLength,
    int32_t sampleLoopEnd, int8_t loopFlag, int8_t sampleIs16Bit, int32_t position)
{
    voice_t *v;

    v = &voice[i];

    if ((sampleData == NULL) || (sampleLength < 1))
    {
        v->mixRoutine = NULL; /* shut down voice */
        return;
    }

    if (sampleIs16Bit)
    {
        sampleLoopBegin  /= 2;
        sampleLength     /= 2;
        sampleLoopLength /= 2;
        sampleLoopEnd    /= 2;

        v->sampleData16 = (const int16_t *)(sampleData);
    }
    else
    {
        v->sampleData8 = sampleData;
    }

    if (sampleLoopLength < 1)
        loopFlag = false;

    v->backwards = false;
    v->SLen      = loopFlag ? sampleLoopEnd : sampleLength;
    v->SRepS     = sampleLoopBegin;
    v->SRepL     = sampleLoopLength;
    v->SPos      = position;
    v->SPosDec   = 0; /* position fraction */

    /* test if 9xx position overflows */
    if (v->SPos >= v->SLen)
    {
        v->mixRoutine = NULL; /* shut down voice */
        return;
    }

    v->mixRoutine = mixRoutineTable[(sampleIs16Bit * 12) + (volumeRampingFlag * 6) + (interpolationFlag * 3) + loopFlag];
}

static void mix_SaveIPVolumes(void) /* for volume ramping */
{
    int32_t i;
    voice_t *v;

    for (i = 0; i < song.antChn; ++i)
    {
        v = &voice[i];

        v->SLVol2 = v->SLVol1;
        v->SRVol2 = v->SRVol1;

        v->SVolIPLen = 0;
    }
}

static void mix_UpdateChannelVolPanFrq(void)
{
    uint8_t i, status;
    uint16_t vol;
    stmTyp *ch;
    sampleTyp *s;
    voice_t *v;

    for (i = 0; i < song.antChn; ++i)
    {
        ch = &stm[i];
        v  = &voice[i];

        status = ch->status;
        if (status != 0)
        {
            ch->status = 0;

            if (status & IS_Vol)
            {
                vol = ch->finalVol;
                if (vol > 0) /* yes, FT2 does this! now it's 0..255 instead */
                    vol--;

                v->SVol = (uint8_t)(vol);
            }

            if (status & IS_Pan)
                v->SPan = ch->finalPan;

            if (status & (IS_Vol | IS_Pan))
                voiceUpdateVolumes(i, status);

            if (status & IS_Period)
                v->SFrq = getFrequenceValue(ch->finalPeriod);

            if (status & IS_NyTon)
            {
                s = ch->smpPtr;

                voiceSetSource(i, s->pek, s->len, s->repS, s->repL, s->repS + s->repL, s->typ & 3,
                    (s->typ & 16) >> 4, ch->smpStartPos);
            }
        }
    }
}

/* ----------------------------------------------------------------------- */
/*                          GENERAL MIXER MACROS                           */
/* ----------------------------------------------------------------------- */

#define GET_VOL \
    CDA_LVol = v->SLVol2; \
    CDA_RVol = v->SRVol2; \

#define SET_VOL_BACK \
    v->SLVol2 = CDA_LVol; \
    v->SRVol2 = CDA_RVol; \

#define GET_MIXER_VARS \
    audioMixL  = mixBufferL; \
    audioMixR  = mixBufferR; \
    mixInMono  = (CDA_LVol == CDA_RVol); \
    realPos    = v->SPos; \
    pos        = v->SPosDec; \
    delta      = v->SFrq; \

#define GET_MIXER_VARS_RAMP \
    audioMixL  = mixBufferL; \
    audioMixR  = mixBufferR; \
    CDA_LVolIP = v->SLVolIP; \
    CDA_RVolIP = v->SRVolIP; \
    mixInMono  = (v->SLVol2 == v->SRVol2) && (CDA_LVolIP == CDA_RVolIP); \
    realPos    = v->SPos; \
    pos        = v->SPosDec; \
    delta      = v->SFrq; \

#define SET_BASE8 \
    CDA_LinearAdr = v->sampleData8; \
    smpPtr = CDA_LinearAdr + realPos; \

#define SET_BASE16 \
    CDA_LinearAdr = v->sampleData16; \
    smpPtr = CDA_LinearAdr + realPos; \

#define INC_POS \
    pos += delta; \
    smpPtr += (pos >> 16); \
    pos &= 0xFFFF; \

#define DEC_POS \
    pos += delta; \
    smpPtr -= (pos >> 16); \
    pos &= 0xFFFF; \

#define SET_BACK_MIXER_POS \
    v->SPosDec = pos; \
    v->SPos = realPos; \

/* ----------------------------------------------------------------------- */
/*                          SAMPLE RENDERING MACROS                        */
/* ----------------------------------------------------------------------- */

/* linear interpolation */
#define _LERP8(s1, s2, f) /* s1,s2 = -128..127 | f = 0..65535 (frac) - 8-bit precision */ \
    s2 -= s1; \
    s2 *= (f); \
    s2 >>= (16 - 8); \
    s1 <<= 8; \
    s1 += s2; \

#define _LERP16(s1, s2, f) /* s1,s2 = -32768..32767 | f = 0..65535 (frac) - 15-bit precision */ \
    s2 -= s1; \
    s2 >>= 1; \
    s2 *= (f); \
    s2 >>= (16 - 1); \
    s1 += s2; \

/* all the 64-bit MULs here convert to fast logic on most 32-bit CPUs */

#define RENDER_8BIT_SMP \
    sample = (*smpPtr) << (28 - 8); \
    *audioMixL++ += (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixR++ += (int32_t)(((int64_t)(sample) * CDA_RVol) >> 32); \

#define RENDER_8BIT_SMP_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP8(sample, sample2, pos) \
    sample <<= (28 - 16); \
    *audioMixL++ += (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixR++ += (int32_t)(((int64_t)(sample) * CDA_RVol) >> 32); \

#define RENDER_8BIT_SMP_LERP_BACKWARDS \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP8(sample, sample2, pos ^ 0xFFFF) \
    sample <<= (28 - 16); \
    *audioMixL++ += (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixR++ += (int32_t)(((int64_t)(sample) * CDA_RVol) >> 32); \

#define RENDER_8BIT_SMP_MONO \
    sample = (*smpPtr) << (28 - 8); \
    sample = (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_8BIT_SMP_MONO_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP8(sample, sample2, pos) \
    sample <<= (28 - 16); \
    sample = (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_8BIT_SMP_MONO_LERP_BACKWARDS \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP8(sample, sample2, pos ^ 0xFFFF) \
    sample <<= (28 - 16); \
    sample = (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_16BIT_SMP \
    sample = (*smpPtr) << (28 - 16); \
    *audioMixL++ += (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixR++ += (int32_t)(((int64_t)(sample) * CDA_RVol) >> 32); \

#define RENDER_16BIT_SMP_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP16(sample, sample2, pos) \
    sample <<= (28 - 16); \
    *audioMixL++ += (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixR++ += (int32_t)(((int64_t)(sample) * CDA_RVol) >> 32); \

#define RENDER_16BIT_SMP_LERP_BACKWARDS \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP16(sample, sample2, pos ^ 0xFFFF) \
    sample <<= (28 - 16); \
    *audioMixL++ += (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixR++ += (int32_t)(((int64_t)(sample) * CDA_RVol) >> 32); \

#define RENDER_16BIT_SMP_MONO \
    sample = (*smpPtr) << (28 - 16); \
    sample = (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_16BIT_SMP_MONO_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP16(sample, sample2, pos) \
    sample <<= (28 - 16); \
    sample = (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_16BIT_SMP_MONO_LERP_BACKWARDS \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP16(sample, sample2, pos ^ 0xFFFF) \
    sample <<= (28 - 16); \
    sample = (int32_t)(((int64_t)(sample) * CDA_LVol) >> 32); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define VOLUME_RAMPING \
    CDA_LVol += CDA_LVolIP; \
    CDA_RVol += CDA_RVolIP; \

/* ----------------------------------------------------------------------- */
/*                      SAMPLES-TO-MIX LIMITING MACROS                     */
/* ----------------------------------------------------------------------- */

#define LIMIT_MIX_NUM \
    CDA_SmpEndFlag = true; \
    \
    i = (v->SLen - 1) - realPos; \
    if (v->SFrq > (i >> 16)) \
    { \
        if (i > 65535) /* won't fit in a 32-bit div */ \
        { \
            samplesToMix = ((uint32_t)(pos ^ 0xFFFFFFFF) / v->SFrq) + 1; \
            CDA_SmpEndFlag = false; \
        } \
        else \
        { \
            samplesToMix = ((uint32_t)((i << 16) | (pos ^ 0x0000FFFF)) / v->SFrq) + 1; \
        } \
    } \
    else \
    { \
        samplesToMix = 65535; \
    } \
    \
    if (samplesToMix > (uint32_t)(CDA_BytesLeft)) \
    { \
        samplesToMix = CDA_BytesLeft; \
        CDA_SmpEndFlag = false; \
    } \

#define LIMIT_MIX_NUM_BIDI_LOOP \
    CDA_SmpEndFlag = true; \
    \
    if (v->backwards) \
        i = realPos - v->SRepS; \
    else \
        i = (v->SLen - 1) - realPos; \
    \
    if (v->SFrq > (i >> 16)) \
    { \
        if (i > 65535) /* won't fit in a 32-bit div */ \
        { \
            samplesToMix = ((uint32_t)(pos ^ 0xFFFFFFFF) / v->SFrq) + 1; \
            CDA_SmpEndFlag = false; \
        } \
        else \
        { \
            samplesToMix = ((uint32_t)((i << 16) | (pos ^ 0x0000FFFF)) / v->SFrq) + 1; \
        } \
    } \
    else \
    { \
        samplesToMix = 65535; \
    } \
    \
    if (samplesToMix > (uint32_t)(CDA_BytesLeft)) \
    { \
        samplesToMix = CDA_BytesLeft; \
        CDA_SmpEndFlag = false; \
    } \

#define LIMIT_MIX_NUM_RAMP \
    if (v->SVolIPLen == 0) \
    { \
        CDA_LVolIP = 0; \
        CDA_RVolIP = 0; \
        \
        if (v->isFadeOutVoice) \
        { \
            v->mixRoutine = NULL; /* fade out voice is done, shut it down */ \
            return; \
        } \
    } \
    else \
    { \
        if (samplesToMix > (uint32_t)(v->SVolIPLen)) \
        { \
            samplesToMix = v->SVolIPLen; \
            CDA_SmpEndFlag = false; \
        } \
        \
        v->SVolIPLen -= samplesToMix; \
    } \

/* ----------------------------------------------------------------------- */
/*                     SAMPLE END/LOOP WRAPPING MACROS                     */
/* ----------------------------------------------------------------------- */

#define HANDLE_SAMPLE_END \
    realPos = (int32_t)(smpPtr - CDA_LinearAdr); \
    if (CDA_SmpEndFlag) \
    { \
        v->mixRoutine = NULL; \
        return; \
    } \

#define WRAP_LOOP \
    realPos = (int32_t)(smpPtr - CDA_LinearAdr); \
    if (CDA_SmpEndFlag) \
    { \
        do \
        { \
            realPos -= v->SRepL; \
        } \
        while (realPos >= v->SLen); \
        \
        smpPtr = CDA_LinearAdr + realPos; \
    } \

#define WRAP_BIDI_LOOP \
    realPos = (int32_t)(smpPtr - CDA_LinearAdr); \
    if (CDA_SmpEndFlag) \
    { \
        /* if backwards: turn backward underflow into forward overflow */ \
        if (v->backwards) \
            realPos = (v->SLen - 1) + (v->SRepS - realPos); \
        \
        do \
        { \
            realPos -= v->SRepL; \
            v->backwards ^= 1; \
        } \
        while (realPos >= v->SLen); \
        \
        /* if backwards: forwards position -> backwards position */ \
        if (v->backwards) \
            realPos = (v->SLen - 1) - (realPos - v->SRepS); \
        \
        smpPtr = CDA_LinearAdr + realPos; \
    } \

/* ----------------------------------------------------------------------- */
/*                       VOLUME=0 OPTIMIZATION MACROS                      */
/* ----------------------------------------------------------------------- */

#define VOL0_OPTIMIZATION_NO_LOOP \
    pos     = v->SPosDec + ((v->SFrq & 0xFFFF) * numSamples); \
    realPos = v->SPos    + ((v->SFrq >>    16) * numSamples) + (pos >> 16); \
    pos    &= 0xFFFF; \
    \
    if (realPos >= v->SLen) \
    { \
        v->mixRoutine = NULL; /* shut down voice */ \
        return; \
    } \
    \
    SET_BACK_MIXER_POS

#define VOL0_OPTIMIZATION_LOOP \
    pos     = v->SPosDec + ((v->SFrq & 0xFFFF) * numSamples); \
    realPos = v->SPos    + ((v->SFrq >>    16) * numSamples) + (pos >> 16); \
    pos    &= 0xFFFF; \
    \
    while (realPos >= v->SLen) \
           realPos -= v->SRepL; \
    \
    SET_BACK_MIXER_POS

#define VOL0_OPTIMIZATION_BIDI_LOOP \
    /* if backwards: backwards position -> forwards position. */ \
    /* in FT2, we're always inside the loop when sampling backwards */ \
    if (v->backwards) \
        v->SPos = (v->SLen - 1) - (v->SPos - v->SRepS); \
    \
    pos     = v->SPosDec + ((v->SFrq & 0xFFFF) * numSamples); \
    realPos = v->SPos    + ((v->SFrq >>    16) * numSamples) + (pos >> 16); \
    pos    &= 0xFFFF; \
    \
    while (realPos >= v->SLen) \
    { \
        realPos -= v->SRepL; \
        v->backwards ^= 1; \
    } \
    \
    /* if backwards: forwards position -> backwards position */ \
    if (v->backwards) \
        realPos = (v->SLen - 1) - (realPos - v->SRepS); \
    \
    SET_BACK_MIXER_POS

/* ----------------------------------------------------------------------- */
/*                          8-BIT MIXING ROUTINES                          */
/* ----------------------------------------------------------------------- */

static void mix8bNoLoop(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_MONO
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO
                INC_POS
                RENDER_8BIT_SMP_MONO
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP
                INC_POS
                RENDER_8BIT_SMP
                INC_POS
            }
        }

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix8bLoop(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_MONO
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO
                INC_POS
                RENDER_8BIT_SMP_MONO
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP
                INC_POS
                RENDER_8BIT_SMP
                INC_POS
            }
        }

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix8bBidiLoop(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        CDA_BytesLeft -= samplesToMix;

        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO
                    DEC_POS
                    RENDER_8BIT_SMP_MONO
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP
                    DEC_POS
                    RENDER_8BIT_SMP
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO
                    INC_POS
                    RENDER_8BIT_SMP_MONO
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP
                    INC_POS
                    RENDER_8BIT_SMP
                    INC_POS
                }
            }
        }

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix8bNoLoopLerp(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_MONO_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO_LERP
                INC_POS
                RENDER_8BIT_SMP_MONO_LERP
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_LERP
                INC_POS
                RENDER_8BIT_SMP_LERP
                INC_POS
            }
        }

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix8bLoopLerp(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_MONO_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO_LERP
                INC_POS
                RENDER_8BIT_SMP_MONO_LERP
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_LERP
                INC_POS
                RENDER_8BIT_SMP_LERP
                INC_POS
            }
        }

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix8bBidiLoopLerp(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        CDA_BytesLeft -= samplesToMix;

        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO_LERP_BACKWARDS
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO_LERP_BACKWARDS
                    DEC_POS
                    RENDER_8BIT_SMP_MONO_LERP_BACKWARDS
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_LERP_BACKWARDS
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_LERP_BACKWARDS
                    DEC_POS
                    RENDER_8BIT_SMP_LERP_BACKWARDS
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO_LERP
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO_LERP
                    INC_POS
                    RENDER_8BIT_SMP_MONO_LERP
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_LERP
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_LERP
                    INC_POS
                    RENDER_8BIT_SMP_LERP
                    INC_POS
                }
            }
        }

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix8bRampNoLoop(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix8bRampLoop(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix8bRampBidiLoop(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_8BIT_SMP_MONO
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_8BIT_SMP
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_8BIT_SMP_MONO
                    VOLUME_RAMPING
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_8BIT_SMP
                    VOLUME_RAMPING
                    INC_POS
                }
            }
        }
        SET_VOL_BACK

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix8bRampNoLoopLerp(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix8bRampLoopLerp(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_8BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_8BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_8BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix8bRampBidiLoopLerp(voice_t *v, uint32_t numSamples)
{
    const int8_t *CDA_LinearAdr;
    uint8_t mixInMono, CDA_SmpEndFlag;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int8_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE8

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_8BIT_SMP_MONO_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_8BIT_SMP_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_MONO_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_MONO_LERP
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_8BIT_SMP_MONO_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_8BIT_SMP_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_8BIT_SMP_LERP
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_8BIT_SMP_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
            }
        }
        SET_VOL_BACK

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

/* ----------------------------------------------------------------------- */
/*                          16-BIT MIXING ROUTINES                         */
/* ----------------------------------------------------------------------- */

static void mix16bNoLoop(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO
                INC_POS
                RENDER_16BIT_SMP_MONO
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP
                INC_POS
                RENDER_16BIT_SMP
                INC_POS
            }
        }

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix16bLoop(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO
                INC_POS
                RENDER_16BIT_SMP_MONO
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP
                INC_POS
                RENDER_16BIT_SMP
                INC_POS
            }
        }

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix16bBidiLoop(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        CDA_BytesLeft -= samplesToMix;

        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO
                    DEC_POS
                    RENDER_16BIT_SMP_MONO
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP
                    DEC_POS
                    RENDER_16BIT_SMP
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO
                    INC_POS
                    RENDER_16BIT_SMP_MONO
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP
                    INC_POS
                    RENDER_16BIT_SMP
                    INC_POS
                }
            }
        }

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix16bNoLoopLerp(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO_LERP
                INC_POS
                RENDER_16BIT_SMP_MONO_LERP
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_LERP
                INC_POS
                RENDER_16BIT_SMP_LERP
                INC_POS
            }
        }

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix16bLoopLerp(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol| CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        CDA_BytesLeft -= samplesToMix;

        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO_LERP
                INC_POS
                RENDER_16BIT_SMP_MONO_LERP
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_LERP
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_LERP
                INC_POS
                RENDER_16BIT_SMP_LERP
                INC_POS
            }
        }

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix16bBidiLoopLerp(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    GET_VOL

    if ((CDA_LVol | CDA_RVol) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        CDA_BytesLeft -= samplesToMix;

        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO_LERP_BACKWARDS
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO_LERP_BACKWARDS
                    DEC_POS
                    RENDER_16BIT_SMP_MONO_LERP_BACKWARDS
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_LERP_BACKWARDS
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_LERP_BACKWARDS
                    DEC_POS
                    RENDER_16BIT_SMP_LERP_BACKWARDS
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO_LERP
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO_LERP
                    INC_POS
                    RENDER_16BIT_SMP_MONO_LERP
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_LERP
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_LERP
                    INC_POS
                    RENDER_16BIT_SMP_LERP
                    INC_POS
                }
            }
        }

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix16bRampNoLoop(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix16bRampLoop(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP_MONO
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix16bRampBidiLoop(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_16BIT_SMP_MONO
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_16BIT_SMP
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_16BIT_SMP_MONO
                    VOLUME_RAMPING
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_16BIT_SMP
                    VOLUME_RAMPING
                    INC_POS
                }
            }
        }
        SET_VOL_BACK

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix16bRampNoLoopLerp(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos,sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int16_t *smpPtr;
    register int32_t  CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        HANDLE_SAMPLE_END
    }

    SET_BACK_MIXER_POS
}

static void mix16bRampLoopLerp(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (mixInMono)
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP_MONO_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        else
        {
            if (samplesToMix & 1)
            {
                RENDER_16BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
            samplesToMix >>= 1;
            for (i = 0; i < samplesToMix; ++i)
            {
                RENDER_16BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
                RENDER_16BIT_SMP_LERP
                VOLUME_RAMPING
                INC_POS
            }
        }
        SET_VOL_BACK

        WRAP_LOOP
    }

    SET_BACK_MIXER_POS
}

static void mix16bRampBidiLoopLerp(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, CDA_SmpEndFlag;
    const int16_t *CDA_LinearAdr;
    int32_t realPos, sample, sample2, *audioMixL, *audioMixR, CDA_BytesLeft, CDA_LVolIP, CDA_RVolIP;
    register const int16_t *smpPtr;
    register int32_t CDA_LVol, CDA_RVol;
    register uint32_t pos, delta;
    uint32_t i, samplesToMix;

    if ((v->SLVol1 | v->SRVol1 | v->SLVol2 | v->SRVol2) == 0)
    {
        VOL0_OPTIMIZATION_BIDI_LOOP
        return;
    }

    GET_MIXER_VARS_RAMP
    SET_BASE16

    CDA_BytesLeft = numSamples;
    while (CDA_BytesLeft > 0)
    {
        LIMIT_MIX_NUM_BIDI_LOOP
        LIMIT_MIX_NUM_RAMP
        CDA_BytesLeft -= samplesToMix;

        GET_VOL
        if (v->backwards)
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_16BIT_SMP_MONO_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                    RENDER_16BIT_SMP_LERP_BACKWARDS
                    VOLUME_RAMPING
                    DEC_POS
                }
            }
        }
        else
        {
            if (mixInMono)
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_MONO_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_MONO_LERP
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_16BIT_SMP_MONO_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
            }
            else
            {
                if (samplesToMix & 1)
                {
                    RENDER_16BIT_SMP_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
                samplesToMix >>= 1;
                for (i = 0; i < samplesToMix; ++i)
                {
                    RENDER_16BIT_SMP_LERP
                    VOLUME_RAMPING
                    INC_POS
                    RENDER_16BIT_SMP_LERP
                    VOLUME_RAMPING
                    INC_POS
                }
            }
        }
        SET_VOL_BACK

        WRAP_BIDI_LOOP
    }

    SET_BACK_MIXER_POS
}

mixRoutine mixRoutineTable[24] =
{
    (mixRoutine)(mix8bNoLoop),
    (mixRoutine)(mix8bLoop),
    (mixRoutine)(mix8bBidiLoop),
    (mixRoutine)(mix8bNoLoopLerp),
    (mixRoutine)(mix8bLoopLerp),
    (mixRoutine)(mix8bBidiLoopLerp),
    (mixRoutine)(mix8bRampNoLoop),
    (mixRoutine)(mix8bRampLoop),
    (mixRoutine)(mix8bRampBidiLoop),
    (mixRoutine)(mix8bRampNoLoopLerp),
    (mixRoutine)(mix8bRampLoopLerp),
    (mixRoutine)(mix8bRampBidiLoopLerp),
    (mixRoutine)(mix16bNoLoop),
    (mixRoutine)(mix16bLoop),
    (mixRoutine)(mix16bBidiLoop),
    (mixRoutine)(mix16bNoLoopLerp),
    (mixRoutine)(mix16bLoopLerp),
    (mixRoutine)(mix16bBidiLoopLerp),
    (mixRoutine)(mix16bRampNoLoop),
    (mixRoutine)(mix16bRampLoop),
    (mixRoutine)(mix16bRampBidiLoop),
    (mixRoutine)(mix16bRampNoLoopLerp),
    (mixRoutine)(mix16bRampLoopLerp),
    (mixRoutine)(mix16bRampBidiLoopLerp)
};

/* ----------------------------------------------------------------------- */

static inline void noNewAllChannels(void)
{
    uint8_t i;

    for (i = 0; i < song.antChn; ++i)
    {
        doEffects(&stm[i]);
        fixaEnvelopeVibrato(&stm[i]);
    }
}

static void getNextPos(void)
{
    if (song.timer == 1)
    {
        song.pattPos++;

        if (song.pattDelTime)
        {
            song.pattDelTime2 = song.pattDelTime;
            song.pattDelTime  = 0;
        }

        if (song.pattDelTime2)
        {
            song.pattDelTime2--;
            if (song.pattDelTime2)
                song.pattPos--;
        }

        if (song.pBreakFlag)
        {
            song.pBreakFlag = false;
            song.pattPos    = song.pBreakPos;
        }

        if ((song.pattPos >= song.pattLen) || song.posJumpFlag)
        {
            song.pattPos     = song.pBreakPos;
            song.pBreakPos   = 0;
            song.posJumpFlag = false;

            if (++song.songPos >= song.len)
                  song.songPos  = song.repS;

            song.pattNr  = song.songTab[song.songPos & 0xFF];
            song.pattLen = pattLens[song.pattNr & 0xFF];
        }
    }
}

static void mainPlayer(void) /* periodically called from mixer */
{
    uint8_t i, readNewNote;

    if (!songPlaying)
    {
        for (i = 0; i < song.antChn; ++i)
            fixaEnvelopeVibrato(&stm[i]);
    }
    else
    {
        readNewNote = false;
        if (--song.timer == 0)
        {
            song.timer = song.tempo;
            readNewNote = true;
        }

        if (readNewNote)
        {
            if (!song.pattDelTime2)
            {
                for (i = 0; i < song.antChn; ++i)
                {
                    if (patt[song.pattNr] != NULL)
                        getNewNote(&stm[i], &patt[song.pattNr][(song.pattPos * song.antChn) + i]);
                    else
                        getNewNote(&stm[i], &nilPatternLine);

                    fixaEnvelopeVibrato(&stm[i]);
                }
            }
            else
            {
                noNewAllChannels();
            }
        }
        else
        {
            noNewAllChannels();
        }

        getNextPos();
    }
}

static void stopVoice(uint8_t i)
{
    voice_t *v;

    v = &voice[i];
    memset(v, 0, sizeof(voice_t));
    v->SPan = 128;

    /* clear "fade out" voice too */

    v = &voice[MAX_VOICES + i];
    memset(v, 0, sizeof(voice_t));
    v->SPan = 128;
}

static void stopVoices(void)
{
    uint8_t i;
    stmTyp *ch;

    for (i = 0; i < song.antChn; ++i)
    {
        ch = &stm[i];

        ch->tonTyp   = 0;
        ch->relTonNr = 0;
        ch->instrNr  = 0;
        ch->instrPtr = instr[0]; /* placeholder instrument */
        ch->status   = IS_Vol;
        ch->realVol  = 0;
        ch->outVol   = 0;
        ch->oldVol   = 0;
        ch->finalVol = 0;
        ch->oldPan   = 128;
        ch->outPan   = 128;
        ch->finalPan = 128;
        ch->vibDepth = 0;
        ch->smpPtr   = NULL;

        stopVoice(i);
    }
}

static void setPos(int16_t songPos, int16_t pattPos)
{
    if (songPos > -1)
    {
        song.songPos = songPos;
        if ((song.len > 0) && (song.songPos >= song.len))
            song.songPos = song.len - 1;

        song.pattNr = song.songTab[songPos];
        song.pattLen = pattLens[song.pattNr];
    }

    if (pattPos > -1)
    {
        song.pattPos = pattPos;
        if (song.pattPos >= song.pattLen)
            song.pattPos  = song.pattLen - 1;
    }

    song.timer = 1;
}

static void freeInstr(uint16_t ins)
{
    uint8_t i;

    if (instr[ins] != NULL)
    {
        for (i = 0; i < 16; ++i)
        {
            if (instr[ins]->samp[i].pek != NULL)
            {
                free(instr[ins]->samp[i].pek);
                instr[ins]->samp[i].pek = NULL;
            }
        }

        free(instr[ins]);
        instr[ins] = NULL;
    }
}

static void freeAllInstr(void)
{
    uint16_t i;

    for (i = 0; i < 128; ++i)
        freeInstr(i);
}

static void freeAllPatterns(void)
{
    uint16_t i;

    for (i = 0; i < 256; ++i)
    {
        if (patt[i] != NULL)
        {
            free(patt[i]);
            patt[i] = NULL;
        }
    }
}

static int8_t allocateInstr(uint16_t i)
{
    uint8_t j;
    instrTyp *p;

    if (instr[i] == NULL)
    {
        p = (instrTyp *)(calloc(1, sizeof (instrTyp)));
        if (p == NULL) return (0);

        for (j = 0; j < 16; ++j)
        {
            p->samp[j].pan = 128;
            p->samp[j].vol = 64;
        }

        instr[i] = p;

        return (1);
    }

    return (0);
}

static void delta2Samp(int8_t *p, uint32_t len, uint8_t typ)
{
    int8_t *p8, news8, olds8L, olds8R;
    int16_t *p16, news16, olds16L, olds16R;
    uint32_t i;

    if (typ & 16) len /= 2; /* 16-bit */
    if (typ & 32) len /= 2; /* stereo */

    if (typ & 32)
    {
        if (typ & 16)
        {
            p16 = (int16_t *)(p);

            olds16L = 0;
            olds16R = 0;

            for (i = 0; i < len; ++i)
            {
                news16  = p16[i] + olds16L;
                p16[i]  = news16;
                olds16L = news16;

                news16 = p16[len + i] + olds16R;
                p16[len + i] = news16;
                olds16R = news16;
            }
        }
        else
        {
            p8 = (int8_t *)(p);

            olds8L = 0;
            olds8R = 0;

            for (i = 0; i < len; ++i)
            {
                news8  = p8[i] + olds8L;
                p8[i]  = news8;
                olds8L = news8;

                news8 = p8[len + i] + olds8R;
                p8[len + i] = news8;
                olds8R = news8;
            }
        }
    }
    else
    {
        if (typ & 16)
        {
            p16 = (int16_t *)(p);

            olds16L = 0;

            for (i = 0; i < len; ++i)
            {
                news16  = p16[i] + olds16L;
                p16[i]  = news16;
                olds16L = news16;
            }
        }
        else
        {
            p8 = (int8_t *)(p);

            olds8L = 0;

            for (i = 0; i < len; ++i)
            {
                news8  = p8[i] + olds8L;
                p8[i]  = news8;
                olds8L = news8;
            }
        }
    }
}

static int8_t loadInstrHeader(MEM *f, uint16_t i)
{
    uint8_t j;
    uint32_t readSize;
    instrHeaderTyp ih;
    sampleTyp *s;

    memset(&ih, 0, INSTR_HEADER_SIZE);

    mread(&ih.instrSize, 4, 1, f);

    readSize = ih.instrSize;
    if ((readSize < 4) || (readSize > INSTR_HEADER_SIZE))
        readSize = INSTR_HEADER_SIZE;

    mread(ih.name, readSize - 4, 1, f); /* -4 = skip ih.instrSize */

    /* FT2 bugfix: skip instrument header data if instrSize is above INSTR_HEADER_SIZE */
    if (ih.instrSize > INSTR_HEADER_SIZE)
        mseek(f, ih.instrSize - INSTR_HEADER_SIZE, SEEK_CUR);

    if (ih.antSamp > 16)
        return (false);

    if (ih.antSamp > 0)
    {
        if (!allocateInstr(i))
            return (false);

        /* sanitize stuff for malicious instruments */
        if (ih.mute     !=    1) ih.mute        = 0;
        if (ih.vibDepth >  0x0F) ih.vibDepth    = 0x0F;
        if (ih.vibRate  >  0x3F) ih.vibRate     = 0x3F;
        if (ih.vibTyp   >     3) ih.vibTyp      = 0;

        for (j = 0; j < 96; ++j)
        {
            if (ih.ta[j] > 0x0F)
                ih.ta[j] = 0x0F;
        }
        /*--------------------- */

        /* copy over final instrument data */
        memcpy(instr[i], ih.ta, INSTR_SIZE);
        instr[i]->antSamp = ih.antSamp;

        if (instr[i]->envVPAnt > 12) instr[i]->envVPAnt = 12;
        if (instr[i]->envVRepS > 11) instr[i]->envVRepS = 11;
        if (instr[i]->envVRepE > 11) instr[i]->envVRepE = 11;
        if (instr[i]->envVSust > 11) instr[i]->envVSust = 11;
        if (instr[i]->envPPAnt > 12) instr[i]->envPPAnt = 12;
        if (instr[i]->envPRepS > 11) instr[i]->envPRepS = 11;
        if (instr[i]->envPRepE > 11) instr[i]->envPRepE = 11;
        if (instr[i]->envPSust > 11) instr[i]->envPSust = 11;

        mread(ih.samp, ih.antSamp * sizeof (sampleHeaderTyp), 1, f);

        for (j = 0; j < ih.antSamp; ++j)
        {
            s = &instr[i]->samp[j];

            memcpy(s, &ih.samp[j], 12 + 4 + 24);

            /* sanitize stuff for malicious modules */
            if (s->vol > 64) s->vol = 64;
            s->relTon = CLAMP(s->relTon, -48, 71);
        }
    }

    return (true);
}

/* adds wrapped sample after loop/end (for branchless linear interpolation) */
static void fixSample(sampleTyp *s)
{
    uint8_t loopType;
    int16_t *ptr16;
    int32_t len;

    len = s->len;
    if (s->pek == NULL)
        return; /* empty sample */

    loopType = s->typ & 3;
    if (loopType == 0)
    {
        /* no loop */

        if (s->typ & 16)
        {
            if (len < 2)
                return;

            ptr16 = (int16_t *)(s->pek);
            ptr16[len / 2] = 0;
        }
        else
        {
            if (len < 1)
                return;

            s->pek[len] = 0;
        }
    }
    else if (loopType == 1)
    {
        /* forward loop */

        if (s->typ & 16)
        {
            /* 16-bit sample */

            if (s->repL < 2)
                return;

            ptr16 = (int16_t *)(s->pek);
            ptr16[(s->repS + s->repL) / 2] = ptr16[s->repS / 2];
        }
        else
        {
            /* 8-bit sample */

            if (s->repL < 1)
                return;

            s->pek[s->repS + s->repL] = s->pek[s->repS];
        }
    }
    else
    {
        /* bidi loop */

        if (s->typ & 16)
        {
            /* 16-bit sample */

            if (s->repL < 2)
                return;

            ptr16 = (int16_t *)(s->pek);
            ptr16[(s->repS + s->repL) / 2] = ptr16[((s->repS + s->repL) / 2) - 1];
        }
        else
        {
            /* 8-bit sample */

            if (s->repL < 1)
                return;

            s->pek[s->repS + s->repL] = s->pek[(s->repS + s->repL) - 1];
        }
    }
}

static void checkSampleRepeat(sampleTyp *s)
{
    if (s->repS < 0) s->repS = 0;
    if (s->repL < 0) s->repL = 0;
    if (s->repS > s->len) s->repS = s->len;
    if ((s->repS + s->repL) > s->len) s->repL = s->len - s->repS;

    if (s->repL == 0) s->typ &= ~3; /* non-FT2 fix: force loop off if looplen is 0 */
}

static int8_t loadInstrSample(MEM *f, uint16_t i)
{
    uint16_t j;
    int32_t l;
    sampleTyp *s;

    if (instr[i] == NULL)
        return (true); /* empty instrument */

    for (j = 0; j < instr[i]->antSamp; ++j)
    {
        s = &instr[i]->samp[j];

        /* if a sample has both forward loop and bidi loop set, make it bidi loop only */
        if ((s->typ & 3) == 3)
            s->typ &= 0xFE;

        l = s->len;
        if (l <= 0)
        {
            s->pek  = NULL;
            s->len  = 0;
            s->repL = 0;
            s->repS = 0;

            if (s->typ & 32)
                s->typ &= ~32; /* remove stereo flag */
        }
        else
        {
            s->pek = (int8_t *)(malloc(l + 2));
            if (s->pek == NULL)
                return (false);

            mread(s->pek, l, 1, f);
            delta2Samp(s->pek, l, s->typ);

            if (s->typ & 32) /* stereo sample - already downmixed to mono in delta2samp() */
            {
                s->typ &= ~32; /* remove stereo flag */

                s->len  /= 2;
                s->repL /= 2;
                s->repS /= 2;

                s->pek = (int8_t *)(realloc(s->pek, s->len + 2));
            }
        }

        /* NON-FT2 FIX: Align to 2-byte if 16-bit sample */
        if (s->typ & 16)
        {
            s->repL &= 0xFFFFFFFE;
            s->repS &= 0xFFFFFFFE;
            s->len  &= 0xFFFFFFFE;
        }

        checkSampleRepeat(s);
        fixSample(s);
    }

    return (true);
}

static void unpackPatt(uint8_t *dst, uint16_t inn, uint16_t len, uint8_t antChn)
{
    uint8_t note, data, *src;
    int32_t i, j, srcEnd, srcIdx;

    if (dst == NULL)
        return;

    src    = dst + inn;
    srcEnd = len * (sizeof (tonTyp) * antChn);
    srcIdx = 0;

    for (i = 0; i < len; ++i)
    {
        for (j = 0; j < antChn; ++j)
        {
            if (srcIdx >= srcEnd)
                return; /* error! */

            note = *src++;
            if (note & 0x80)
            {
                data = 0; if (note & 0x01) data = *src++; *dst++ = data;
                data = 0; if (note & 0x02) data = *src++; *dst++ = data;
                data = 0; if (note & 0x04) data = *src++; *dst++ = data;
                data = 0; if (note & 0x08) data = *src++; *dst++ = data;
                data = 0; if (note & 0x10) data = *src++; *dst++ = data;
            }
            else
            {
                *dst++ = note;
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
            }

            /* Non-FT2 security fix: if effect is above 35 (Z), clear effect and parameter */
            if (*(dst - 2) > 35)
            {
                *(dst - 2) = 0;
                *(dst - 1) = 0;
            }

            srcIdx += sizeof (tonTyp);
        }
    }
}

static int8_t patternEmpty(uint16_t nr)
{
    uint8_t *scanPtr;
    uint32_t i, scanLen;

    if (patt[nr] == NULL)
        return (true);

    scanPtr = (uint8_t *)(patt[nr]);
    scanLen = pattLens[nr] * (5 * song.antChn);

    for (i = 0; i < scanLen; ++i)
    {
        if (scanPtr[i] != 0)
            return (false);
    }

    return (true);
}

static int8_t loadPatterns(MEM *f, uint16_t antPtn)
{
    uint8_t tmpLen, *pattPtr;
    uint16_t i, a;
    patternHeaderTyp ph;

    for (i = 0; i < antPtn; ++i)
    {
        mread(&ph.patternHeaderSize, 4, 1, f);
        mread(&ph.typ, 1, 1, f);

        ph.pattLen = 0;
        if (song.ver == 0x0102)
        {
            mread(&tmpLen, 1, 1, f);
            mread(&ph.dataLen, 2, 1, f);
            ph.pattLen = (uint16_t)(tmpLen) + 1; /* +1 in v1.02 */

            if (ph.patternHeaderSize > 8)
                mseek(f, ph.patternHeaderSize - 8, SEEK_CUR);
        }
        else
        {
            mread(&ph.pattLen, 2, 1, f);
            mread(&ph.dataLen, 2, 1, f);

            if (ph.patternHeaderSize > 9)
                mseek(f, ph.patternHeaderSize - 9, SEEK_CUR);
        }

        if (meof(f))
        {
            mclose(&f);
            return (false);
        }

        pattLens[i] = ph.pattLen;
        if (ph.dataLen)
        {
            a = ph.pattLen * (sizeof (tonTyp) * song.antChn);

            patt[i] = (tonTyp *)(malloc(a + 16)); /* + 16 = a little extra for safety */
            if (patt[i] == NULL)
                return (false);

            pattPtr = (uint8_t *)(patt[i]);

            memset(pattPtr, 0, a);
            mread(&pattPtr[a - ph.dataLen], 1, ph.dataLen, f);
            unpackPatt(pattPtr, a - ph.dataLen, ph.pattLen, song.antChn);
        }

        if (patternEmpty(i))
        {
            if (patt[i] != NULL)
            {
                free(patt[i]);
                patt[i] = NULL;
            }

            pattLens[i] = 64;
        }

        if (pattLens[i] > 256)
            pattLens[i] = 64;
    }

    return (true);
}

static int8_t loadMusicMOD(MEM *f, uint32_t fileLength)
{
    uint8_t bytes[4], modIsUST, modIsFEST, modIsNT;
    int16_t i, j, k, ai;
    uint16_t a, b, period;
    tonTyp *ton;
    sampleTyp *s;
    songMOD31HeaderTyp h_MOD31;
    songMOD15HeaderTyp h_MOD15;

    const char modSig[32][5] =
    {
        "1CHN", "2CHN", "3CHN", "4CHN", "5CHN", "6CHN", "7CHN", "8CHN",
        "9CHN", "10CH", "11CH", "12CH", "13CH", "14CH", "15CH", "16CH",
        "17CH", "18CH", "19CH", "20CH", "21CH", "22CH", "23CH", "24CH",
        "25CH", "26CH", "27CH", "28CH", "29CH", "30CH", "31CH", "32CH"
    };

    /* start loading MOD */

    if ((fileLength < 1596) || (fileLength > 20842494)) /* minimum and maximum possible size for an FT2 .mod */
    {
        mclose(&f);
        return (false);
    }

    mseek(f, 0, SEEK_SET);

    if (mread(&h_MOD31, sizeof (h_MOD31), 1, f) != 1)
    {
        mclose(&f);
        return (false);
    }

    modIsFEST = false;
    modIsNT   = false;

    if (!strncmp(h_MOD31.sig, "N.T.", 4))
    {
        j = 4;
        modIsNT = true;
    }
    else if (!strncmp(h_MOD31.sig, "FEST", 4) || !strncmp(h_MOD31.sig, "M&K!", 4))
    {
        modIsFEST = true;
        modIsNT   = true;
        j = 4;
    }
    else if (!strncmp(h_MOD31.sig, "M!K!", 4) || !strncmp(h_MOD31.sig, "M.K.", 4) ||
             !strncmp(h_MOD31.sig, "FLT4", 4))
    {
        j = 4;
    }
    else if (!strncmp(h_MOD31.sig, "OCTA", 4) || !strncmp(h_MOD31.sig, "FLT8", 4) || !strncmp(h_MOD31.sig, "CD81", 4))
    {
        j = 8;
    }
    else
    {
        j = 0;
        for (i = 0; i < 32; ++i)
        {
            if (!strncmp(h_MOD31.sig, modSig[i], 4))
            {
                j = i + 1;
                break;
            }
            else
            {
                if (j == 31)
                    j = -1; /* ID not recignized */
            }
        }
    }

    /* unsupported MOD */
    if (j == -1)
    {
        mclose(&f);
        return (false);
    }

    if (j > 0)
    {
        modIsUST = false;
        if (fileLength < sizeof (h_MOD31))
        {
            mclose(&f);
            return (false);
        }

        song.antChn = (uint8_t)(j);
        song.len    = h_MOD31.len;
        song.repS   = h_MOD31.repS;

        memcpy(song.songTab, h_MOD31.songTab, 128);
        ai = 31;
    }
    else
    {
        modIsUST = true;
        if (fileLength < sizeof (h_MOD15))
        {
            mclose(&f);
            return (false);
        }

        mseek(f, 0, SEEK_SET);
        if (mread(&h_MOD15, sizeof (h_MOD15), 1, f) != 1)
        {
            mclose(&f);
            return (false);
        }

        song.antChn = 4;
        song.len = h_MOD15.len;
        song.repS = h_MOD15.repS;
        memcpy(song.songTab, h_MOD15.songTab, 128);
        ai = 15;
    }

    if ((song.antChn < 1) || (song.antChn > MAX_VOICES) || (song.len < 1))
    {
        mclose(&f);
        return (false);
    }

    if (!strncmp(h_MOD31.sig, "M.K.", 4) && (song.len == 129))
        song.len = 127; /* fixes beatwave.mod by Sidewinder */

    if (song.len > 128)
    {
        mclose(&f);
        return (false);
    }

    if (modIsUST && ((song.repS == 0) || (song.repS > 220)))
    {
        mclose(&f);
        return (false);
    }

    /* trim off spaces at end of name */
    for (i = 19; i >= 0; --i)
    {
        if ((h_MOD31.name[i] == ' ') || (h_MOD31.name[i] == 0x1A))
            h_MOD31.name[i] = '\0';
        else
            break;
    }

    memcpy(song.name, h_MOD31.name, 20);
    song.name[20] = '\0';

    b = 0;
    for (a = 0; a < 128; ++a)
    {
        if (song.songTab[a] > 99)
        {
            mclose(&f);
            return (false);
        }

        if (song.songTab[a] > b)
            b = song.songTab[a];
    }

    for (a = 0; a <= b; ++a)
    {
        patt[a] = (tonTyp *)(calloc(64 * song.antChn, sizeof (tonTyp)));
        if (patt[a] == NULL)
        {
            freeAllPatterns();
            mclose(&f);
            return (false);
        }

        pattLens[a] = 64;

        for (j = 0; j < 64; ++j)
        {
            for (k = 0; k < song.antChn; ++k)
            {
                ton = &patt[a][(j * song.antChn) + k];
                mread(bytes, 1, 4, f);

                /* period to note */
                period = (((bytes[0] & 0x0F) << 8) | bytes[1]) & 0x0FFF;
                for (i = 0; i < (8 * 12); ++i)
                {
                    if (period >= amigaPeriod[i])
                    {
                        ton->ton = (uint8_t)(1 + i);
                        break;
                    }
                }

                ton->instr = (bytes[0] & 0xF0) | (bytes[2] >> 4);
                ton->effTyp = bytes[2] & 0x0F;
                ton->eff = bytes[3];

                if (ton->effTyp == 0xC)
                {
                    if (ton->eff > 64)
                        ton->eff = 64;
                }
                else if (ton->effTyp == 0x1)
                {
                    if (ton->eff == 0)
                        ton->effTyp = 0;
                }
                else if (ton->effTyp == 0x2)
                {
                    if (ton->eff == 0)
                        ton->effTyp = 0;
                }
                else if (ton->effTyp == 0x5)
                {
                    if (ton->eff == 0)
                        ton->effTyp = 0x3;
                }
                else if (ton->effTyp == 0x6)
                {
                    if (ton->eff == 0)
                        ton->effTyp = 0x4;
                }
                else if (ton->effTyp == 0xA)
                {
                    if (ton->eff == 0)
                        ton->effTyp = 0;
                }
                else if (ton->effTyp == 0xE)
                {
                    /* check if certain E commands are empty */
                    if ((ton->eff == 0x10) || (ton->eff == 0x20) || (ton->eff == 0xA0) || (ton->eff == 0xB0))
                    {
                        ton->effTyp = 0;
                        ton->eff    = 0;
                    }
                }

                if (modIsUST)
                {
                    if (ton->effTyp == 0x01)
                    {
                        /* arpeggio */
                        ton->effTyp = 0x00;
                    }
                    else if (ton->effTyp == 0x02)
                    {
                        /* pitch slide */
                        if (ton->eff & 0xF0)
                        {
                            ton->effTyp = 0x02;
                            ton->eff >>= 4;
                        }
                        else if (ton->eff & 0x0F)
                        {
                            ton->effTyp = 0x01;
                        }
                    }

                    if ((ton->effTyp == 0x0D) && (ton->eff > 0))
                        ton->effTyp = 0x0A;
                }

                if (modIsNT && (ton->effTyp == 0x0D))
                    ton->eff = 0;
            }
        }

        if (patternEmpty(a))
            pattLens[a] = 64;
    }

    for (a = 0; a < ai; ++a)
    {
        if (!allocateInstr(1 + a))
        {
            freeAllPatterns();
            freeAllInstr();
            mclose(&f);
            return (false);
        }

        if (h_MOD31.instr[a].len > 0)
        {
            s = &instr[1 + a]->samp[0];

            s->len = 2 * SWAP16(h_MOD31.instr[a].len);

            s->pek = (int8_t *)(malloc(s->len + 2));
            if (s->pek == NULL)
            {
                freeAllPatterns();
                freeAllInstr();
                mclose(&f);
                return (false);
            }

            if (modIsFEST)
                h_MOD31.instr[a].fine = (32 - (h_MOD31.instr[a].fine & 0x1F)) / 2;

            if (!modIsUST)
                s->fine = 8 * ((2 * ((h_MOD31.instr[a].fine & 0x0F) ^ 8)) - 16);
            else
                s->fine = 0;

            s->pan = 128;

            s->vol = h_MOD31.instr[a].vol;
            if (s->vol > 64) s->vol = 64;

            s->repS = 2 * SWAP16(h_MOD31.instr[a].repS);

            if (modIsUST)
                s->repS /= 2;

            s->repL = 2 * SWAP16(h_MOD31.instr[a].repL);

            if (s->repL <= 2)
            {
                s->repS = 0;
                s->repL = 0;
            }

            if ((s->repS + s->repL) > s->len)
            {
                if (s->repS >= s->len)
                {
                    s->repS = 0;
                    s->repL = 0;
                }
                else
                {
                    s->repL = s->len - s->repS;
                }
            }

            if (s->repL > 2)
                s->typ = 1;
            else
                s->typ = 0;

            if (!modIsUST)
            {
                mread(s->pek, 1, s->len, f);
            }
            else
            {
                if ((s->repS > 2) && (s->repS < s->len))
                {
                    s->len -= s->repS;

                    mseek(f, s->repS, SEEK_CUR);
                    mread(s->pek, 1, s->len, f);

                    s->repS = 0;
                }
                else
                {
                    mread(s->pek, 1, s->len, f);
                }
            }

            fixSample(s);
        }
    }

    if (modIsUST)
    {
        /* repS is initialBPM in UST MODs */
        if (song.repS == 120)
        {
            song.speed = 125;
        }
        else
        {
            if (song.repS > 239)
                song.repS = 239;

            song.speed = (uint16_t)(1773447 / ((240 - song.repS) * 122));
        }

        song.repS = 0;
    }
    else
    {
        song.speed = 125;
        if (song.repS >= song.len)
            song.repS = 0;
    }

    mclose(&f);

    song.tempo = 6;

    note2Period = amigaPeriods;

    if (song.repS > song.len)
        song.repS = 0;

    stopVoices();
    setPos(0, 0);

    /* instr 0 is a placeholder for invalid instruments */
    allocateInstr(0);
    instr[0]->samp[0].vol = 0;
    /* ------------------------------------------------ */

    moduleLoaded = true;
    return (true);
}

static int8_t loadMusic(const uint8_t *moduleData, uint32_t dataLength)
{
    uint16_t i;
    songHeaderTyp h;
    MEM *f;

    freeAllInstr();
    freeAllPatterns();
    memset(&song, 0, sizeof (song));

    moduleLoaded = false;

    linearFrqTab = 0;

    f = mopen(moduleData, dataLength);
    if (f == NULL) return (0);

    /* start loading */
    mread(&h, sizeof (h), 1, f);

    if (memcmp(h.sig, "Extended Module: ", 17) != 0)
        return (loadMusicMOD(f, dataLength));

    if ((h.ver < 0x0102) || (h.ver > 0x104))
    {
        mclose(&f);
        return (0);
    }

    if ((h.len > 256) || (h.antChn < 1) || (h.antChn > MAX_VOICES) || (h.antPtn > 256))
    {
        mclose(&f);
        return (0);
    }

    mseek(f, 60 + h.headerSize, SEEK_SET);
    if (meof(f))
    {
        mclose(&f);
        return (0);
    }

    song.len     = h.len;
    song.repS    = h.repS;
    song.antChn  = (uint8_t)(h.antChn);
    song.speed   = h.defSpeed ? h.defSpeed : 125;
    song.tempo   = h.defTempo ? h.defTempo : 6;
    song.ver     = h.ver;
    linearFrqTab = h.flags & 1;

    song.speed = CLAMP(song.speed, 32, 255);

    if (song.tempo > 31)
        song.tempo = 31;

    if (song.globVol > 64)
        song.globVol = 64;

    if (song.len == 0)
        song.len = 1; /* songTmp.songTab is already empty */
    else
        memcpy(song.songTab, h.songTab, song.len);

    if (song.ver < 0x0104)
    {
        /* old FT2 format */

        for (i = 1; i <= h.antInstrs; ++i)
        {
            if (!loadInstrHeader(f, i))
            {
                freeAllInstr();
                mclose(&f);
                return (0);
            }
        }

        if (!loadPatterns(f, h.antPtn))
        {
            freeAllInstr();
            mclose(&f);
            return (0);
        }

        for (i = 1; i <= h.antInstrs; ++i)
        {
            if (!loadInstrSample(f, i))
            {
                freeAllInstr();
                mclose(&f);
                return (0);
            }
        }
    }
    else
    {
        /* current FT2 format */

        if (!loadPatterns(f, h.antPtn))
        {
            mclose(&f);
            return (0);
        }

        for (i = 1; i <= h.antInstrs; ++i)
        {
            if (!loadInstrHeader(f, i))
            {
                freeInstr((uint8_t)(i));
                mclose(&f);
                break;
            }

            if (!loadInstrSample(f, i))
            {
                mclose(&f);
                break;
            }
        }
    }

    mclose(&f);

    note2Period = linearFrqTab ? linearPeriods : amigaPeriods;

    if (song.repS > song.len)
        song.repS = 0;

    setSpeed(song.speed);
    stopVoices();
    setPos(0, 0);

    /* instr 0 is a placeholder for invalid instruments */
    allocateInstr(0);
    instr[0]->samp[0].vol = 0;
    /* ------------------------------------------------ */

    moduleLoaded = true;
    return (true);
}

void ft2play_SetMasterVol(uint16_t vol)
{
    if (vol > 256)
        vol = 256;

    masterVol = vol;
}

void ft2play_SetAmp(uint8_t ampFactor)
{
    int32_t i, newAmp;

    newAmp = CLAMP(ampFactor, 1, 32);
    newAmp = (256 * newAmp) / 32;

    if (amp != newAmp)
    {
        amp = newAmp;

        /* make all channels update volume because of amp change */
        for (i = 0; i < song.antChn; ++i)
            stm[i].status |= IS_Vol;
    }
}

void ft2play_SetInterpolation(uint8_t flag)
{
    interpolationFlag = flag ? true : false;
    stopVoices();
}

void ft2play_SetVolumeRamping(uint8_t flag)
{
    volumeRampingFlag = flag ? true : false;
    stopVoices();
}

char *ft2play_GetSongName(void)
{
    return (song.name);
}

static void mixAudio(int16_t *stream, int32_t sampleBlockLength)
{
    int32_t i, out32;
    voice_t *v;

    if (musicPaused)
    {
        memset(stream, 0, sampleBlockLength * sizeof (int16_t) * 2);
        return;
    }

    memset(mixBufferL, 0, sampleBlockLength * sizeof (int32_t));
    memset(mixBufferR, 0, sampleBlockLength * sizeof (int32_t));

    /* mix channels */
    for (i = 0; i < song.antChn; ++i)
    {
        /* mix normal voice */
        v = &voice[i];

        /* call the mixing routine currently set for the voice */
        if (v->mixRoutine != NULL)
           (v->mixRoutine)((void *)(v), sampleBlockLength);

        /* mix fade-out voice */
        v = &voice[MAX_VOICES + i];

        /* call the mixing routine currently set for the voice */
        if (v->mixRoutine != NULL)
           (v->mixRoutine)((void *)(v), sampleBlockLength);
    }

    for (i = 0; i < sampleBlockLength; ++i)
    {
        /* left channel */
        out32 = ((mixBufferL[i] >> 8) * masterVol) >> 8;
        CLAMP16(out32); /* FAST 16-bit clamp technique */
        *stream++ = (int16_t)(out32);

        /* right channel */
        out32 = ((mixBufferR[i] >> 8) * masterVol) >> 8;
        CLAMP16(out32);
        *stream++ = (int16_t)(out32);
    }
}

static void ft2play_FillAudioBuffer(int16_t *buffer, int32_t samples)
{
    int32_t a, b;

    a = samples;
    while (a > 0)
    {
        if (pmpLeft == 0)
        {
            /* new replayer tick */
            if (!musicPaused)
            {
                if (volumeRampingFlag)
                    mix_SaveIPVolumes();

                mainPlayer();
                mix_UpdateChannelVolPanFrq();
            }

            pmpLeft = speedVal;
        }

        b = a;
        if (b > pmpLeft)
            b = pmpLeft;

        mixAudio(buffer, b);
        buffer += (b * 2);

        a -= b;
        pmpLeft -= b;
    }

    sampleCounter += samples;
}

void ft2play_Close(void)
{
    closeMixer();

    freeAllInstr();
    freeAllPatterns();

    if (mixBufferL != NULL)
    {
        free(mixBufferL);
        mixBufferL = NULL;
    }

    if (mixBufferR != NULL)
    {
        free(mixBufferR);
        mixBufferR = NULL;
    }

    if (logTab != NULL)
    {
        free(logTab);
        logTab = NULL;
    }

    if (vibSineTab != NULL)
    {
        free(vibSineTab);
        vibSineTab = NULL;
    }

    if (amigaPeriods != NULL)
    {
        free(amigaPeriods);
        amigaPeriods = NULL;
    }

    if (linearPeriods != NULL)
    {
        free(linearPeriods);
        linearPeriods = NULL;
    }
}

void ft2play_PauseSong(int8_t flag)
{
    musicPaused = flag;
}

void ft2play_TogglePause(void)
{
    musicPaused ^= 1;
}

uint32_t ft2play_GetMixerTicks(void)
{
    if (realReplayRate < 1000)
        return (0);

    return (sampleCounter / (realReplayRate / 1000));
}

int8_t ft2play_PlaySong(const uint8_t *moduleData, uint32_t dataLength, int8_t useInterpolationFlag, int8_t useVolumeRampingFlag, uint32_t audioFreq)
{
    uint8_t j;
    int16_t noteVal;
    uint16_t i, noteIndex;

    if (audioFreq == 0)
        audioFreq = 44100;

    audioFreq = CLAMP(audioFreq, 11025, 96000);

    musicPaused = true;

    ft2play_Close();
    memset(song.name, 0, sizeof (song.name));

    sampleCounter = 0;

    /* initialize replayer and mixer */

    mixBufferL = (int32_t *)(malloc(MIX_BUF_SAMPLES * sizeof (int32_t)));
    mixBufferR = (int32_t *)(malloc(MIX_BUF_SAMPLES * sizeof (int32_t)));

    if ((mixBufferL == NULL) || (mixBufferR == NULL))
    {
        ft2play_Close();
        return (false);
    }

    /* allocate memory for pointers */

    if (linearPeriods == NULL)
        linearPeriods = (int16_t *)(malloc(sizeof (int16_t) * ((12 * 10 * 16) + 16)));

    if (amigaPeriods == NULL)
        amigaPeriods = (int16_t *)(malloc(sizeof (int16_t) * ((12 * 10 * 16) + 16)));

    if (vibSineTab == NULL)
        vibSineTab = (int8_t *)(malloc(256));

    if (logTab == NULL)
        logTab = (uint32_t *)(malloc(sizeof (uint32_t) * 768));

    if ((linearPeriods == NULL) || (amigaPeriods == NULL) ||
        (vibSineTab    == NULL) || (logTab       == NULL))
    {
        ft2play_Close();
        return (false);
    }

    /* generate tables */

    /* generate log table (value-exact to FT2's table) */
    for (i = 0; i < (4 * 12 * 16); ++i)
        logTab[i] = (uint32_t)(round(16777216.0 * exp((i / 768.0) * M_LN2)));

    /* generate linear table (value-exact to FT2's table) */
    for (i = 0; i < ((12 * 10 * 16) + 16); ++i)
        linearPeriods[i] = (((12 * 10 * 16) + 16) * 4) - (i * 4);

    /* generate amiga period table (value-exact to FT2's table, except for last 17 entries) */
    noteIndex = 0;
    for (i = 0; i < 10; ++i)
    {
        for (j = 0; j < ((i == 9) ? (96 + 8) : 96); ++j)
        {
            noteVal = ((amigaFinePeriod[j % 96] * 64) + (-1 + (1 << i))) >> (i + 1);
            /* NON-FT2: j % 96. added for safety. we're patching the values later anyways. */

            amigaPeriods[noteIndex++] = noteVal;
            amigaPeriods[noteIndex++] = noteVal;
        }
    }

    /* interpolate between points (end-result is exact to FT2's table, except for last 17 entries) */
    for (i = 0; i < (12 * 10 * 8) + 7; ++i)
        amigaPeriods[(i * 2) + 1] = (amigaPeriods[i * 2] + amigaPeriods[(i * 2) + 2]) / 2;

    /*
    ** the amiga linear period table has its 17 last entries generated wrongly.
    ** the content seem to be garbage because of an 'out of boundaries' read from AmigaFinePeriods.
    ** these 17 values were taken from a memdump of FT2 in DOSBox.
    ** they might change depending on what you ran before FT2, but let's not make it too complicated.
    */

    amigaPeriods[1919] = 22; amigaPeriods[1920] = 16; amigaPeriods[1921] =  8; amigaPeriods[1922] =  0;
    amigaPeriods[1923] = 16; amigaPeriods[1924] = 32; amigaPeriods[1925] = 24; amigaPeriods[1926] = 16;
    amigaPeriods[1927] =  8; amigaPeriods[1928] =  0; amigaPeriods[1929] = 16; amigaPeriods[1930] = 32;
    amigaPeriods[1931] = 24; amigaPeriods[1932] = 16; amigaPeriods[1933] =  8; amigaPeriods[1934] =  0;
    amigaPeriods[1935] =  0;

    /* generate auto-vibrato table (value-exact to FT2's table) */
    for (i = 0; i < 256; ++i)
        vibSineTab[i] = (int8_t)(floor((64.0 * sin((-i * (2.0 * M_PI)) / 256.0)) + 0.5));

    if (!loadMusic(moduleData, dataLength))
    {
        ft2play_Close();
        return (false);
    }

    realReplayRate    = audioFreq;
    soundBufferSize   = MIX_BUF_SAMPLES * 4; /* samples -> bytes */
    interpolationFlag = useInterpolationFlag ? true : false;
    volumeRampingFlag = useVolumeRampingFlag ? true : false;

    /* for voice delta calculation */
    frequenceDivFactor = (uint32_t)(round(65536.0 *  1712.0 / realReplayRate * 8363.0));
    frequenceMulFactor = (uint32_t)(round(  256.0 * 65536.0 / realReplayRate * 8363.0));

    /* for volume ramping */
    quickVolSizeVal = realReplayRate / 200;

    ft2play_SetAmp(10);
    ft2play_SetMasterVol(256);

    stopVoices();

    song.globVol = 64;

    if (song.speed == 0)
        song.speed = 125;

    setSpeed(song.speed);

    setPos(0, 0);
    songPlaying = true;

    if (!openMixer(realReplayRate))
    {
        ft2play_Close();
        return (false);
    }

    musicPaused = false;
    return (true);
}

static MEM *mopen(const uint8_t *src, uint32_t length)
{
    MEM *b;

    if ((src == NULL) || (length == 0))
        return (NULL);

    b = (MEM *)(malloc(sizeof (MEM)));
    if (b == NULL)
        return (NULL);

    b->_base   = (uint8_t *)(src);
    b->_ptr    = (uint8_t *)(src);
    b->_cnt    = length;
    b->_bufsiz = length;
    b->_eof    = false;
 
    return (b);
}

static void mclose(MEM **buf)
{
    if (*buf != NULL)
    {
        free(*buf);
        *buf = NULL;
    }
}

static size_t mread(void *buffer, size_t size, size_t count, MEM *buf)
{
    size_t wrcnt;
    int32_t pcnt;

    if ((buf == NULL) || (buf->_ptr == NULL))
        return (0);

    wrcnt = size * count;
    if ((size == 0) || buf->_eof)
        return (0);

    pcnt = ((uint32_t)(buf->_cnt) > wrcnt) ? wrcnt : buf->_cnt;
    memcpy(buffer, buf->_ptr, pcnt);

    buf->_cnt -= pcnt;
    buf->_ptr += pcnt;

    if (buf->_cnt <= 0)
    {
        buf->_ptr = buf->_base + buf->_bufsiz;
        buf->_cnt = 0;
        buf->_eof = true;
    }

    return (pcnt / size);
}

static int32_t meof(MEM *buf)
{
    if (buf == NULL)
        return (true);

    return (buf->_eof);
}

static void mseek(MEM *buf, int32_t offset, int32_t whence)
{
    if (buf == NULL)
        return;

    if (buf->_base)
    {
        switch (whence)
        {
            case SEEK_SET: buf->_ptr  = buf->_base + offset;                break;
            case SEEK_CUR: buf->_ptr += offset;                             break;
            case SEEK_END: buf->_ptr  = buf->_base + buf->_bufsiz + offset; break;
            default: break;
        }

        buf->_eof = false;
        if (buf->_ptr >= (buf->_base + buf->_bufsiz))
        {
            buf->_ptr = buf->_base + buf->_bufsiz;
            buf->_eof = true;
        }

        buf->_cnt = (buf->_base + buf->_bufsiz) - buf->_ptr;
    }
}

/* the following must be changed if you want to use another audio API than WinMM */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <mmsystem.h>

#define MIX_BUF_NUM 2

static volatile BOOL audioRunningFlag;
static uint8_t currBuffer;
static int16_t *mixBuffer[MIX_BUF_NUM];
static HANDLE hThread, hAudioSem;
static WAVEHDR waveBlocks[MIX_BUF_NUM];
static HWAVEOUT hWave;

static DWORD WINAPI mixThread(LPVOID lpParam)
{
    WAVEHDR *waveBlock;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    while (audioRunningFlag)
    {
        waveBlock = &waveBlocks[currBuffer];
        ft2play_FillAudioBuffer((int16_t *)(waveBlock->lpData), MIX_BUF_SAMPLES);
        waveOutWrite(hWave, waveBlock, sizeof (WAVEHDR));
        currBuffer = (currBuffer + 1) % MIX_BUF_NUM;

        /* wait for buffer fill request */
        WaitForSingleObject(hAudioSem, INFINITE);
    }

    (void)(lpParam); /* make compiler happy! */

    return (0);
}

static void CALLBACK waveProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (uMsg == WOM_DONE)
        ReleaseSemaphore(hAudioSem, 1, NULL);

    /* make compiler happy! */
    (void)(hWaveOut);
    (void)(uMsg);
    (void)(dwInstance);
    (void)(dwParam1);
    (void)(dwParam2);
}

static void closeMixer(void)
{
    int32_t i;

    audioRunningFlag = false; /* make thread end when it's done */

    if (hAudioSem != NULL)
        ReleaseSemaphore(hAudioSem, 1, NULL);

    if (hThread != NULL)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
    }

    if (hAudioSem != NULL)
    {
        CloseHandle(hAudioSem);
        hAudioSem = NULL;
    }

    if (hWave != NULL)
    {
        waveOutReset(hWave);

        for (i = 0; i < MIX_BUF_NUM; ++i)
        {
            if (waveBlocks[i].dwUser != 0xFFFF)
                waveOutUnprepareHeader(hWave, &waveBlocks[i], sizeof (WAVEHDR));
        }

        waveOutClose(hWave);
        hWave = NULL;
    }

    for (i = 0; i < MIX_BUF_NUM; ++i)
    {
        if (mixBuffer[i] != NULL)
        {
            free(mixBuffer[i]);
            mixBuffer[i] = NULL;
        }
    }
}

static int8_t openMixer(uint32_t audioFreq)
{
    int32_t i;
    DWORD threadID;
    WAVEFORMATEX wfx;

    /* don't unprepare headers on error */
    for (i = 0; i < MIX_BUF_NUM; ++i)
        waveBlocks[i].dwUser = 0xFFFF;

    closeMixer();

    ZeroMemory(&wfx, sizeof (wfx));
    wfx.nSamplesPerSec  = audioFreq;
    wfx.wBitsPerSample  = 16;
    wfx.nChannels       = 2;
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nBlockAlign     = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    pmpLeft    = 0;
    currBuffer = 0;

    if (waveOutOpen(&hWave, WAVE_MAPPER, &wfx, (DWORD_PTR)(&waveProc), 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
        goto omError;

    /* create semaphore for buffer fill requests */
    hAudioSem = CreateSemaphore(NULL, MIX_BUF_NUM - 1, MIX_BUF_NUM, NULL);
    if (hAudioSem == NULL)
        goto omError;

    /* allocate WinMM mix buffers */
    for (i = 0; i < MIX_BUF_NUM; ++i)
    {
        mixBuffer[i] = (int16_t *)(calloc(MIX_BUF_SAMPLES, wfx.nBlockAlign));
        if (mixBuffer[i] == NULL)
            goto omError;
    }

    /* initialize WinMM mix headers */
    memset(waveBlocks, 0, sizeof (waveBlocks));
    for (i = 0; i < MIX_BUF_NUM; ++i)
    {
        waveBlocks[i].lpData = (LPSTR)(mixBuffer[i]);
        waveBlocks[i].dwBufferLength = MIX_BUF_SAMPLES * wfx.nBlockAlign;
        waveBlocks[i].dwFlags = WHDR_DONE;

        if (waveOutPrepareHeader(hWave, &waveBlocks[i], sizeof (WAVEHDR)) != MMSYSERR_NOERROR)
            goto omError;
    }

    /* create main mixer thread */
    audioRunningFlag = true;
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(mixThread), NULL, 0, &threadID);
    if (hThread == NULL)
        goto omError;

    return (TRUE);

omError:
    closeMixer();
    return (FALSE);
}

/* --------------------------------------------------------------------------- */

/* END OF FILE (phew...) */