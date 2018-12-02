/*
** ST3PLAY v0.88 - 28th of November 2018 - https://16-bits.org
** ===========================================================
**                 - NOT BIG ENDIAN SAFE! -
**
** Very accurate C port of Scream Tracker 3.21's replayer,
** by Olav "8bitbubsy" Sørensen. Using the original asm source codes
** by Sami "PSI" Tammilehto (Future Crew) with permission.
**
** This replayer supports 16-bit samples (which ST3 doesn't)!
**
** You need to link winmm.lib for this to compile (-lwinmm)
** Alternatively, you can change out the mixer functions at the bottom with
** your own for your OS.
**
** Example of st3play usage:
** #include "st3play.h"
** #include "songdata.h"
**
** st3play_PlaySong(songData, songDataLength, true, SOUNDCARD_GUS, 44100);
** mainLoop();
** st3play_Close();
**
** To turn a song into an include file like in the example, you can use my win32
** bin2h tool from here: https://16-bits.org/etc/bin2h.zip
**
** Changes in v0.88:
** - Rewrote the S3M loader
**
** Changes in v0.87:
** - More audio channel mixer optimizations
**
** Changes in v0.86:
** - Fixed GUS panning positions (now uses a table)
**
** Changes in v0.85:
** - Removed all 64-bit calculations, and made the mixer slightly faster
** - Some code was rewritten to be more correctly ported from the original code
** - st3play_SetMasterVol() now sets mixing vol. instead of the song's mastervol
** - Small code cleanup
**
** Changes in v0.84:
** - Linear interpolation is done with 16-bit frac. precision instead of 15-bit
**
** Changes in v0.83:
** - Prevent stuck loop if order list contains separators (254) only
** - Added a function to retrieve song name
** - Added a function to set master volume (0..256)
**
** Changes in v0.82:
** - Fixed an error in loop wrapping in the audio channel mixer
** - Audio channel mixer is now optimized and fast!
** - WinMM mixer has been rewritten to be safe (DON'T use syscalls in callback -MSDN)
** - Some small changes to the st3play functions (easier to use and safer!)
** - Removed all non-ST3 stuff (replayer should act identical to ST3 now).
**   You should use another replayer if you want the non-ST3 features.
** - Some small fixes to the replayer and mixer functions
*/

/* st3play.h:

#ifndef __ST3PLAY_H
#define __ST3PLAY_H

#include <stdint.h>

enum
{
    SOUNDCARD_GUS = 0, // default
    SOUNDCARD_SB  = 1
};

int8_t st3play_PlaySong(const uint8_t *moduleData, uint32_t dataLength, uint8_t useInterpolationFlag, uint8_t soundCardType, uint32_t audioFreq);
void st3play_Close(void);
void st3play_PauseSong(int8_t flag); // true/false
void st3play_TogglePause(void);
void st3play_SetMasterVol(uint16_t vol); // 0..256
void st3play_SetInterpolation(uint8_t flag); // true/false
char *st3play_GetSongName(void); // max 28 chars (29 with '\0'), string is in code page 437
uint32_t st3play_GetMixerTicks(void); // returns the amount of milliseconds of mixed audio (not realtime)

#endif
*/

#define MIX_BUF_SAMPLES 4096

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define LERP(x, y, z) ((x) + ((y) - (x)) * (z))

/* fast 32-bit -> 16-bit clamp */
#define CLAMP16(i) \
    if ((int16_t)(i) != i) \
        i = 0x7FFF ^ (i >> 31); \

enum
{
    SOUNDCARD_GUS   = 0, /* default */
    SOUNDCARD_SBPRO = 1,
    PATT_SEP = 254,
    PATT_END = 255,
};

/* STRUCTS */

typedef void (*mixRoutine)(void *, int32_t);

static struct
{
    int8_t *data, vol;
    uint8_t type, flags;
    uint16_t c2spd;
    uint32_t length, loopbeg, looplen;
} ins[100];

typedef struct
{
    int8_t aorgvol, avol, apanpos;
    uint8_t channelnum, amixtype, achannelused, aglis, atremor, atreon, atrigcnt, anotecutcnt, anotedelaycnt;
    uint8_t avibtretype, note, ins, vol, cmd, info, lastins, lastnote, alastnfo, alasteff, alasteff1;
    int16_t avibcnt, asldspd, aspd, aorgspd;
    uint16_t astartoffset, astartoffset00, ac2spd;
} chn_t;

typedef struct
{
    const int8_t *m_base8;
    const int16_t *m_base16;
    uint8_t m_loopflag;
    uint16_t m_vol_l, m_vol_r;
    uint32_t m_pos, m_end, m_looplen, m_posfrac, m_speed;
    void (*m_mixfunc)(void *, int32_t); /* function pointer to mix routine */
} voice_t;

typedef void (*effect_routine)(chn_t *ch);

/* STATIC DATA */
static char songname[28 + 1];
static int8_t **smpPtrs, volslidetype, patterndelay, patloopcount, musicPaused;
static int8_t lastachannelused, oldstvib, fastvolslide, amigalimits, stereomode;
static uint8_t order[256], chnsettings[32], *patdata[100], *np_patseg;
static uint8_t musicmax, soundcardtype, breakpat, startrow, musiccount, interpolationFlag;
static int16_t jmptoord, np_ord, np_row, np_pat, np_patoff, patloopstart, jumptorow, globalvol, aspdmin, aspdmax;
static uint16_t useglobalvol, patmusicrand, ordNum, insNum, patNum;
static int32_t mastermul, mastervol = 256, samplesLeft, soundBufferSize, *mixBufferL, *mixBufferR;
static uint32_t samplesPerTick, audioRate, sampleCounter;
static chn_t chn[32];
static voice_t voice[32];
static mixRoutine mixRoutineTable[8];

/* 8bitbubsy: this panning table was made sampling audio from my GUS PnP and processing it.
** It was scaled from 0..1 to 0..960 to fit the mixing volumes used for SB Pro mixing (4-bit pan * 16) */
static const int16_t guspantab[16] = { 0, 245, 365, 438, 490, 562, 628, 682, 732, 827, 775, 847, 879, 909, 935, 960 };

/* TABLES */
static const int8_t retrigvoladd[32] =
{
    0, -1, -2, -4, -8,-16,  0,  0,
    0,  1,  2,  4,  8, 16,  0,  0,
    0,  0,  0,  0,  0,  0, 10,  8,
    0,  0,  0,  0,  0,  0, 24, 32
};

static const uint8_t octavediv[16] = 
{
    0, 1, 2, 3, 4, 5, 6, 7,

    /* overflow data from xvol_amiga table */
    0, 5, 11, 17, 27, 32, 42, 47
};

static const int16_t notespd[16] =
{
    1712 * 16, 1616 * 16, 1524 * 16,
    1440 * 16, 1356 * 16, 1280 * 16,
    1208 * 16, 1140 * 16, 1076 * 16,
    1016 * 16,  960 * 16,  907 * 16,
    1712 * 8,

    /* overflow data from adlibiadd table */
    0x0100, 0x0802, 0x0A09
};

static const int16_t vibsin[64] =
{
     0x00, 0x18, 0x31, 0x4A, 0x61, 0x78, 0x8D, 0xA1,
     0xB4, 0xC5, 0xD4, 0xE0, 0xEB, 0xF4, 0xFA, 0xFD,
     0xFF, 0xFD, 0xFA, 0xF4, 0xEB, 0xE0, 0xD4, 0xC5,
     0xB4, 0xA1, 0x8D, 0x78, 0x61, 0x4A, 0x31, 0x18,
     0x00,-0x18,-0x31,-0x4A,-0x61,-0x78,-0x8D,-0xA1,
    -0xB4,-0xC5,-0xD4,-0xE0,-0xEB,-0xF4,-0xFA,-0xFD,
    -0xFF,-0xFD,-0xFA,-0xF4,-0xEB,-0xE0,-0xD4,-0xC5,
    -0xB4,-0xA1,-0x8D,-0x78,-0x61,-0x4A,-0x31,-0x18
};

static const uint8_t vibsqu[64] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const int16_t vibramp[64] =
{
       0, -248,-240,-232,-224,-216,-208,-200,
    -192, -184,-176,-168,-160,-152,-144,-136,
    -128, -120,-112,-104, -96, -88, -80, -72,
     -64,  -56, -48, -40, -32, -24, -16,  -8,
       0,    8,  16,  24,  32,  40,  48,  56,
      64,   72,  80,  88,  96, 104, 112, 120,
     128,  136, 144, 152, 160, 168, 176, 184,
     192,  200, 208, 216, 224, 232, 240, 248
};


/* FUNCTION DECLARATIONS */
static void voiceSetSource(uint8_t voiceNumber, const int8_t *sampleData,
    int32_t length, int32_t loopStart, int32_t loopLength,
    uint8_t loopFlag, uint8_t sampleIs16Bit);
static void voiceSetSamplePosition(uint8_t voiceNumber, uint16_t value);
static void voiceSetVolume(uint8_t voiceNumber, uint16_t vol, uint8_t pan);

static void s_ret(chn_t *ch);
static void s_setgliss(chn_t *ch);
static void s_setfinetune(chn_t *ch);
static void s_setvibwave(chn_t *ch);
static void s_settrewave(chn_t *ch);
static void s_settrewave(chn_t *ch);
static void s_setpanpos(chn_t *ch);
static void s_stereocntr(chn_t *ch);
static void s_patloop(chn_t *ch);
static void s_notecut(chn_t *ch);
static void s_notecutb(chn_t *ch);
static void s_notedelay(chn_t *ch);
static void s_notedelayb(chn_t *ch);
static void s_patterdelay(chn_t *ch);
static void s_setspeed(chn_t *ch);
static void s_jmpto(chn_t *ch);
static void s_break(chn_t *ch);
static void s_volslide(chn_t *ch);
static void s_slidedown(chn_t *ch);
static void s_slideup(chn_t *ch);
static void s_toneslide(chn_t *ch);
static void s_vibrato(chn_t *ch);
static void s_tremor(chn_t *ch);
static void s_arp(chn_t *ch);
static void s_vibvol(chn_t *ch);
static void s_tonevol(chn_t *ch);
static void s_retrig(chn_t *ch);
static void s_tremolo(chn_t *ch);
static void s_scommand1(chn_t *ch);
static void s_scommand2(chn_t *ch);
static void s_settempo(chn_t *ch);
static void s_finevibrato(chn_t *ch);
static void s_setgvol(chn_t *ch);

static int8_t openMixer(uint32_t audioFreq);
static void closeMixer(void);

static effect_routine ssoncejmp[16] =
{
    s_ret,         // 0
    s_setgliss,    // 1
    s_setfinetune, // 2
    s_setvibwave,  // 3
    s_settrewave,  // 4
    s_ret,         // 5
    s_ret,         // 6
    s_ret,         // 7
    s_setpanpos,   // 8
    s_ret,         // 9
    s_stereocntr,  // A
    s_patloop,     // B
    s_notecut,     // C
    s_notedelay,   // D
    s_patterdelay, // E
    s_ret          // F
};

static effect_routine ssotherjmp[16] =
{
    s_ret,        // 0
    s_ret,        // 1
    s_ret,        // 2
    s_ret,        // 3
    s_ret,        // 4
    s_ret,        // 5
    s_ret,        // 6
    s_ret,        // 7
    s_ret,        // 8
    s_ret,        // 9
    s_ret,        // A
    s_ret,        // B
    s_notecutb,   // C
    s_notedelayb, // D
    s_ret,        // E
    s_ret         // F
};

static effect_routine soncejmp[27] =
{
    s_ret,       // .
    s_setspeed,  // A
    s_jmpto,     // B
    s_break,     // C
    s_volslide,  // D
    s_slidedown, // E
    s_slideup,   // F
    s_ret,       // G
    s_ret,       // H
    s_tremor,    // I
    s_arp,       // J
    s_ret,       // K
    s_ret,       // L
    s_ret,       // M
    s_ret,       // N
    s_ret,       // O - handled in doamiga()
    s_ret,       // P
    s_retrig,    // Q
    s_ret,       // R
    s_scommand1, // S
    s_settempo,  // T
    s_ret,       // U
    s_ret,       // V
    s_ret,       // W
    s_ret,       // X
    s_ret,       // Y
    s_ret        // Z
};

static effect_routine sotherjmp[27] =
{
    s_ret,         // .
    s_ret,         // A
    s_ret,         // B
    s_ret,         // C
    s_volslide,    // D
    s_slidedown,   // E
    s_slideup,     // F
    s_toneslide,   // G
    s_vibrato,     // H
    s_tremor,      // I
    s_arp,         // J
    s_vibvol,      // K
    s_tonevol,     // L
    s_ret,         // M
    s_ret,         // N
    s_ret,         // O
    s_ret,         // P
    s_retrig,      // Q
    s_tremolo,     // R
    s_scommand2,   // S
    s_ret,         // T
    s_finevibrato, // U
    s_setgvol,     // V
    s_ret,         // W
    s_ret,         // X
    s_ret,         // Y
    s_ret          // Z
};

/* CODE START */

static void getlastnfo(chn_t *ch)
{
    if (ch->info == 0)
        ch->info = ch->alastnfo;
}

static void setspeed(uint8_t val)
{
    if (val > 0)
        musicmax = val;
}

static void settempo(uint8_t val)
{
    if (val > 32)
        samplesPerTick = (audioRate * 125) / (50 * val);
}

static void setspd(chn_t *ch)
{
    int16_t tmpspd;
    uint16_t deltahi, deltalo;
    uint32_t tmp32;

    ch->achannelused |= 0x80;

    if (amigalimits)
    {
        if ((uint16_t)(ch->aorgspd) > aspdmax)
            ch->aorgspd = aspdmax;

        if (ch->aorgspd < aspdmin)
            ch->aorgspd = aspdmin;
    }

    tmpspd = ch->aspd;
    if ((uint16_t)(tmpspd) > aspdmax)
    {
        tmpspd = aspdmax;
        if (amigalimits)
            ch->aspd = tmpspd;
    }

    if (tmpspd == 0)
    {
        voice[ch->channelnum].m_speed = 0; /* cut voice (can be activated again by changing frequency) */
        return;
    }

    if (tmpspd < aspdmin)
    {
        tmpspd = aspdmin;
        if (amigalimits)
            ch->aspd = tmpspd;
    }

    /* ST3 actually uses 14317056 (0xDA7600, main timer 3.579264MHz*4) instead of 14317456 (1712*8363) */

    if (tmpspd <= 0xDA)
    {
        /* this is how ST3 does it for high voice rates, to prevent 64-bit DIV ((tmpspd<<16)/audioRate) */

        tmp32 = ((0xDA / tmpspd) << 16) | ((((0xDA % tmpspd) << 16) | 0x7600) / tmpspd);
        deltahi = (uint16_t)(tmp32 / audioRate);

        tmp32 = ((tmp32 % audioRate) << 16) | deltahi;
        deltalo = (uint16_t)(tmp32 / audioRate);
    }
    else
    {
        tmp32 = 14317056 / tmpspd;

        deltahi = (uint16_t)(tmp32 / audioRate);
        deltalo = (uint16_t)((tmp32 << 16) / audioRate);
    }

    voice[ch->channelnum].m_speed = (deltahi << 16) | deltalo;
}

static void setglobalvol(int8_t vol)
{
    globalvol = vol;

    if ((uint8_t)(vol) > 64)
        vol = 64;

    useglobalvol = globalvol * 4; /* for mixer */
}

static void setvol(chn_t *ch, uint8_t volFlag)
{
    if (volFlag)
        ch->achannelused |= 0x80;

    voiceSetVolume(ch->channelnum, (ch->avol * useglobalvol) >> 8, ch->apanpos & 0x0F);
}

static int16_t stnote2herz(uint8_t note)
{
    uint8_t shiftVal;
    uint16_t noteVal;

    if (note == 254)
        return (0); /* 0hertz/keyoff */

    noteVal = notespd[note & 0x0F];

    shiftVal = octavediv[note >> 4];
    if (shiftVal > 0)
        noteVal >>= (shiftVal & 0x1F);

    return (noteVal);
}

static int16_t scalec2spd(chn_t *ch, int16_t spd)
{
    const int32_t c2freq = 8363;
    int32_t tmpspd;

    tmpspd = spd * c2freq;
    if ((tmpspd >> 16) >= ch->ac2spd)
        return (32767); /* div error */

    tmpspd /= ch->ac2spd;
    if (tmpspd > 32767)
        tmpspd = 32767;

    return ((int16_t)(tmpspd));
}

/* for Gxx with semitones slide flag */
static int16_t roundspd(chn_t *ch, int16_t spd)
{
    const int32_t c2freq = 8363;
    int8_t i, octa, newnote;
    int16_t note, notemin, lastspd;
    int32_t newspd;

    newspd = spd * ch->ac2spd;
    if ((newspd >> 16) >= c2freq)
        return (spd); /* div error */

    newspd /= c2freq;

    /* find octave */

    octa = 0;
    lastspd = (notespd[12] + notespd[11]) >> 1;

    while (lastspd >= newspd)
    {
        octa++;
        lastspd >>= 1;
    }

    /* find note */

    newnote = 0;
    notemin = 32767;

    for (i = 0; i < 11; ++i)
    {
        note = notespd[i];
        if (octa > 0)
            note >>= octa;

        note -= (int16_t)(newspd);
        if (note < 0)
            note = -note;

        if (note < notemin)
        {
            notemin = note;
            newnote = i;
        }
    }

    /* get new speed from new note */

    newspd = (stnote2herz((octa << 4) | (newnote & 0x0F))) * c2freq;
    if ((newspd >> 16) >= ch->ac2spd)
        return (spd); /* div error */

    newspd /= ch->ac2spd;
    return ((int16_t)(newspd));
}

static int16_t neworder(void) /* rewritten to be more safe */
{
    uint8_t patt;
    uint16_t numSeparators;

    numSeparators = 0;
    do
    {
        np_ord++;

        patt = order[np_ord - 1];
        if (patt == PATT_SEP)
        {
            numSeparators++;
            if (numSeparators >= ordNum)
            {
                /* the order list contains separators only, end seeking loop */
                patt = 0;
                np_ord = 0;
                break;
            }
        }

        if ((patt == PATT_END) || (np_ord > ordNum)) /* end reached, start at beginning of order list */
        {
            numSeparators = 0;
            np_ord = 1; /* yes */

            patt = order[0];
            if (patt == PATT_SEP)
                np_ord = 0; /* kludge, since we're going to continue this loop */
        }
    }
    while (patt == PATT_SEP);

    np_pat       = patt;
    np_patoff    = -1; /* force reseek */
    np_row       = startrow;
    startrow     = 0;
    patmusicrand = 0;
    patloopstart = -1;
    jumptorow    = -1;

    return (np_row);
}

static int8_t getnote1(void)
{
    uint8_t dat, channel;
    int16_t i;
    chn_t *ch;

    if ((np_patseg == NULL) || (np_pat >= patNum))
        return (255);

    channel = 0;

    i = np_patoff;
    while (true)
    {
        dat = np_patseg[i++];
        if (dat == 0)
        {
            np_patoff = i;
            return (255);
        }

        if ((chnsettings[dat & 0x1F] & 0x80) == 0)
        {
            channel = dat & 0x1F;
            break;
        }

        /* channel off, skip */
        if (dat & 0x20) i += 2;
        if (dat & 0x40) i += 1;
        if (dat & 0x80) i += 2;
    }

    ch = &chn[channel];

    /* NOTE/INSTRUMENT */
    if (dat & 0x20)
    {
        ch->note = np_patseg[i++];
        ch->ins  = np_patseg[i++];

        if (ch->note != 255) ch->lastnote = ch->note;
        if (ch->ins)         ch->lastins  = ch->ins;
    }

    /* VOLUME */
    if (dat & 0x40)
        ch->vol = np_patseg[i++];

    /* COMMAND/INFO */
    if (dat & 0x80)
    {
        ch->cmd  = np_patseg[i++];
        ch->info = np_patseg[i++];
    }

    np_patoff = i;
    return (channel);
}

static void doamiga(uint8_t channel)
{
    uint8_t smp;
    chn_t *ch;

    ch = &chn[channel];

    if (ch->ins > 0)
        ch->astartoffset = 0;

    /* set sample offset (also for sample triggering) */
    if ((ch->lastins > 0) || (ch->ins > 0))
    {
        if (ch->cmd == ('O' - 64))
        {
            if (!ch->info)
            {
                ch->astartoffset = ch->astartoffset00;
            }
            else
            {
                ch->astartoffset   = ch->info << 8;
                ch->astartoffset00 = ch->astartoffset;
            }
        }

        if (ch->note < 254)
        {
            if ((ch->cmd != ('G' - 64)) && (ch->cmd != ('L' - 64)))
                voiceSetSamplePosition(channel, ch->astartoffset);
        }
    }

    /* ***INSTRUMENT*** */
    if (ch->ins > 0)
    {
        ch->astartoffset = 0;

        ch->lastins = ch->ins;
        if (ch->ins <= insNum) /* 8bitbubsy: added for safety reasons */
        {
            smp = ch->ins - 1;

            if (ins[smp].type > 0)
            {
                if (ins[smp].type == 1) /* sample */
                {
                    ch->ac2spd  = ins[smp].c2spd;
                    ch->avol    = ins[smp].vol;
                    ch->aorgvol = ch->avol;
                    setvol(ch, true);

                    /* this specific portion differs from what sound card driver you use in ST3... */
                    if ((soundcardtype == SOUNDCARD_SBPRO) || ((ch->cmd != ('G' - 64)) && (ch->cmd != ('L' - 64))))
                    {
                        /* on GUS, do no sample swapping without a note number */
                        if ((soundcardtype != SOUNDCARD_GUS) || (ch->note != 255))
                        {
                            voiceSetSource(channel, (const int8_t *)(ins[smp].data), ins[smp].length, ins[smp].loopbeg,
                                ins[smp].looplen, ins[smp].flags & 1, (ins[smp].flags & 4) >> 2);
                        }
                    }
                }
                else
                {
                    /* not sample (adlib instrument?) */
                    ch->lastins = 0;
                }
            }
        }
    }

    /* continue only if we have an active instrument on this channel */
    if (ch->lastins == 0)
        return;

    /* ***NOTE*** */
    if (ch->note != 255)
    {
        if (ch->note == 254)
        {
            /* end sample (not recovarable) */

            ch->aspd = 0;
            setspd(ch);

            ch->avol = 0;
            setvol(ch, true);

            voice[channel].m_mixfunc = NULL;
            voice[channel].m_pos = 0;

            ch->asldspd = 65535; /* 8bitbubsy: most likely a bug */
        }
        else
        {
            /* calc note speed */

            ch->lastnote = ch->note;

            ch->asldspd = scalec2spd(ch, stnote2herz(ch->note)); /* destination for toneslide (G/L) */
            if ((ch->aorgspd == 0) || ((ch->cmd != ('G' - 64)) && (ch->cmd != ('L' - 64))))
            {
                ch->aspd = ch->asldspd;
                setspd(ch);
                ch->avibcnt = 0;
                ch->aorgspd = ch->aspd; /* original speed if true one changed with vibrato etc. */
            }
        }
    }

    /* ***VOLUME*** */
    if (ch->vol != 255)
    {
        ch->avol = ch->vol;
        setvol(ch, true);

        ch->aorgvol = ch->vol;
        ch->aorgvol = ch->vol;
    }
}

static void donewnote(uint8_t channel, int8_t notedelayflag)
{
    chn_t *ch;

    ch = &chn[channel];

    if (notedelayflag)
    {
        ch->achannelused = 0x81;
    }
    else
    {
        if (ch->channelnum > lastachannelused)
        {
            lastachannelused = ch->channelnum + 1;

            /* 8bitbubsy: sanity fix */
            if (lastachannelused > 31)
                lastachannelused = 31;
        }

        ch->achannelused = 0x01;

        if (ch->cmd == ('S' - 64))
        {
            if ((ch->info & 0xF0) == 0xD0)
                return;
        }
    }

    doamiga(channel);
}

static void donotes(void)
{
    uint8_t channel, dat;
    int16_t i, j;
    chn_t *ch;

    for (i = 0; i < 32; ++i)
    {
        ch = &chn[i];

        ch->note = 255;
        ch->vol  = 255;
        ch->ins  = 0;
        ch->cmd  = 0;
        ch->info = 0;
    }

    /* find np_row from pattern */
    if (np_patoff == -1)
    {
        np_patseg = patdata[np_pat];
        if (np_patseg != NULL)
        {
            j = 0;
            if (np_row > 0)
            {
                i = np_row;
                while (i > 0)
                {
                    dat = np_patseg[j++];
                    if (dat == 0)
                    {
                        i--;
                    }
                    else
                    {
                        if (dat & 0x20) j += 2;
                        if (dat & 0x40) j += 1;
                        if (dat & 0x80) j += 2;
                    }
                }
            }

            np_patoff = j;
        }
    }

    while (true)
    {
        channel = getnote1();
        if (channel == 255)
            break; /* end of row/channels */

       if ((chnsettings[channel] & 0x7F) < 16) /* only handle PCM channels for now */
            donewnote(channel, false);
    }
}

/* tick 0 commands */
static void docmd1(void) /* what a mess... */
{
    uint8_t i;
    chn_t *ch;

    for (i = 0; i < (lastachannelused + 1); ++i)
    {
        ch = &chn[i];

        if (ch->achannelused)
        {
            if (ch->info > 0)
                ch->alastnfo = ch->info;

            if (ch->cmd > 0)
            {
                ch->achannelused |= 0x80;

                if (ch->cmd == ('D' - 64))
                {
                    /* fix trigger D */

                    ch->atrigcnt = 0;

                    /* fix speed if tone port noncomplete */
                    if (ch->aspd != ch->aorgspd)
                    {
                        ch->aspd = ch->aorgspd;
                        setspd(ch);
                    }
                }
                else
                {
                    if (ch->cmd != ('I' - 64))
                    {
                        ch->atremor = 0;
                        ch->atreon  = true;
                    }

                    if ((ch->cmd != ('H' - 64)) &&
                        (ch->cmd != ('U' - 64)) &&
                        (ch->cmd != ('K' - 64)) &&
                        (ch->cmd != ('R' - 64)))
                    {
                        ch->avibcnt |= 128;
                    }
                }

                if (ch->cmd < 27)
                {
                    volslidetype = 0;
                    soncejmp[ch->cmd](ch);
                }
            }
            else
            {
                /* fix trigger 0 */

                ch->atrigcnt = 0;

                /* fix speed if tone port noncomplete */
                if (ch->aspd != ch->aorgspd)
                {
                    ch->aspd = ch->aorgspd;
                    setspd(ch);
                }

                if (!amigalimits && (ch->cmd < 27))
                {
                    volslidetype = 0;
                    soncejmp[ch->cmd](ch);
                }
            }
        }
    }
}

static void docmd2(void) /* tick >0 commands */
{
    uint8_t i;
    chn_t *ch;

    for (i = 0; i < (lastachannelused + 1); ++i)
    {
        ch = &chn[i];
        if (ch->achannelused && (ch->cmd > 0))
        {
            ch->achannelused |= 0x80;

            if (ch->cmd < 27)
            {
                volslidetype = 0;
                sotherjmp[ch->cmd](ch);
            }
        }
    }
}

static void dorow(void)
{
    patmusicrand = (((patmusicrand * 0xCDEF) >> 16) + 0x1727) & 0xFFFF;

    if (np_pat == 255)
        return; /* 8bitbubsy: there are no patterns in the song! */

    if (musiccount == 0)
    {
        if (patterndelay > 0)
        {
            np_row--;
            docmd1();
            patterndelay--;
        }
        else
        {
            donotes(); /* new notes */
            docmd1(); /* also does 0volcut */
        }
    }
    else
    {
        docmd2(); /* effects only */
    }

    musiccount++;
    if (musiccount >= musicmax)
    {
        /* next row */

        np_row++;

        if (jumptorow != -1)
        {
            np_row = jumptorow;
            jumptorow = -1;
        }

        if ((np_row >= 64) || ((patloopcount == 0) && (breakpat > 0)))
        {
            /* next pattern */

            if (breakpat == 255)
            {
                breakpat = 0;
                return;
            }

            breakpat = 0;

            if (jmptoord != -1)
            {
                np_ord = jmptoord;
                jmptoord = -1;
            }

            np_row = neworder(); /* if breakpat, np_row = break row */
        }

        musiccount = 0;
    }
}

/* EFFECTS */

static void s_ret(chn_t *ch)
{
    (void)(ch);
}

static void s_setgliss(chn_t *ch)
{
    ch->aglis = ch->info & 0x0F;
}

static void s_setfinetune(chn_t *ch)
{
    /* this has a bug in ST3 that makes this effect do nothing! */
    (void)(ch);
}

static void s_setvibwave(chn_t *ch)
{
    ch->avibtretype = (ch->avibtretype & 0xF0) | ((ch->info << 1) & 0x0F);
}

static void s_settrewave(chn_t *ch)
{
    ch->avibtretype = ((ch->info << 5) & 0xF0) | (ch->avibtretype & 0x0F);
}

static void s_setpanpos(chn_t *ch)
{
    if (soundcardtype == SOUNDCARD_GUS)
    {
        ch->apanpos = ch->info & 0x0F;
        setvol(ch, false);
    }
}

static void s_stereocntr(chn_t *ch)
{
    /* Sound Blaster mix selector (buggy, undocumented ST3 effect):
    ** - SA0 = normal  mix
    ** - SA1 = swapped mix (L<->R)
    ** - SA2 = normal  mix (broken mixing)
    ** - SA3 = swapped mix (broken mixing)
    ** - SA4 = center  mix (broken mixing)
    ** - SA5 = center  mix (broken mixing)
    ** - SA6 = center  mix (broken mixing)
    ** - SA7 = center  mix (broken mixing)
    ** - SA8..F = changes nothing
    */

    if (soundcardtype == SOUNDCARD_SBPRO)
    {
        if ((ch->info & 0x0F) < 8)
        {
            ch->amixtype = ch->info & 0x0F;
            setvol(ch, false);
        }
    }
}

static void s_patloop(chn_t *ch)
{
    if ((ch->info & 0x0F) == 0)
    {
        patloopstart = np_row;
        return;
    }

    if (patloopcount == 0)
    {
        patloopcount = (ch->info & 0x0F) + 1;
        if (patloopstart == -1)
            patloopstart = 0; /* default loopstart */
    }

    if (patloopcount > 1)
    {
        patloopcount--;
        jumptorow = patloopstart;
        np_patoff = -1; /* force reseek */
    }
    else
    {
        patloopcount = 0;
        patloopstart = np_row + 1;
    }
}

static void s_notecut(chn_t *ch)
{
    ch->anotecutcnt = ch->info & 0x0F;
}

static void s_notecutb(chn_t *ch)
{
    if (ch->anotecutcnt > 0)
    {
        ch->anotecutcnt--;
        if (ch->anotecutcnt == 0)
            voice[ch->channelnum].m_speed = 0; /* shut down voice (recoverable by using pitch effects) */
    }
}

static void s_notedelay(chn_t *ch)
{
    ch->anotedelaycnt = ch->info & 0x0F;
}

static void s_notedelayb(chn_t *ch)
{
    if (ch->anotedelaycnt > 0)
    {
        ch->anotedelaycnt--;
        if (ch->anotedelaycnt == 0)
            donewnote(ch->channelnum, true);
    }
}

static void s_patterdelay(chn_t *ch)
{
    if (patterndelay == 0)
        patterndelay = ch->info & 0x0F;
}

static void s_setspeed(chn_t *ch)
{
    setspeed(ch->info);
}

static void s_jmpto(chn_t *ch)
{
    if (ch->info == 0xFF)
    {
        breakpat = 255;
    }
    else
    {
        breakpat = 1;
        jmptoord = ch->info;
    }
}

static void s_break(chn_t *ch)
{
    uint8_t hi, lo;

    hi = ch->info >> 4;
    lo = ch->info & 0x0F;

    if ((hi <= 9) && (lo <= 9))
    {
        startrow = (hi * 10) + lo;
        breakpat = 1;
    }
}

static void s_volslide(chn_t *ch)
{
    uint8_t infohi;
    uint8_t infolo;

    getlastnfo(ch);

    infohi = ch->info >> 4;
    infolo = ch->info & 0x0F;

    if (infolo == 0x0F)
    {
        if (infohi == 0)
            ch->avol -= infolo;
        else if (musiccount == 0)
            ch->avol += infohi;
    }
    else if (infohi == 0x0F)
    {
        if (infolo == 0)
            ch->avol += infohi;
        else if (musiccount == 0)
            ch->avol -= infolo;
    }
    else if (fastvolslide || (musiccount > 0))
    {
        if (infolo == 0)
            ch->avol += infohi;
        else
            ch->avol -= infolo;
    }
    else
    {
        return; /* illegal slide */
    }

    ch->avol = CLAMP(ch->avol, 0, 63);
    setvol(ch, true);

    /* these are set on Kxy/Lxx */
         if (volslidetype == 1) s_vibrato(ch);
    else if (volslidetype == 2) s_toneslide(ch);
}

static void s_slidedown(chn_t *ch)
{
    if (ch->aorgspd > 0)
    {
        getlastnfo(ch);

        if (musiccount > 0)
        {
            if (ch->info >= 0xE0)
                return; /* no fine slides here */

            ch->aspd += (ch->info << 2);
            if ((uint16_t)(ch->aspd) > 32767)
                ch->aspd = 32767;
        }
        else
        {
            if (ch->info <= 0xE0)
                return; /* only fine slides here */

            if (ch->info <= 0xF0)
            {
                ch->aspd += (ch->info & 0x0F);
                if ((uint16_t)(ch->aspd) > 32767)
                    ch->aspd = 32767;
            }
            else
            {
                ch->aspd += ((ch->info & 0x0F) << 2);
                if ((uint16_t)(ch->aspd) > 32767)
                    ch->aspd = 32767;
            }
        }

        ch->aorgspd = ch->aspd;
        setspd(ch);
    }
}

static void s_slideup(chn_t *ch)
{
    if (ch->aorgspd > 0)
    {
        getlastnfo(ch);

        if (musiccount > 0)
        {
            if (ch->info >= 0xE0)
                return; /* no fine slides here */

            ch->aspd -= (ch->info << 2);
            if (ch->aspd < 0)
                ch->aspd = 0;
        }
        else
        {
            if (ch->info <= 0xE0)
                return; /* only fine slides here */

            if (ch->info <= 0xF0)
            {
                ch->aspd -= (ch->info & 0x0F);
                if (ch->aspd < 0)
                    ch->aspd = 0;
            }
            else
            {
                ch->aspd -= ((ch->info & 0x0F) << 2);
                if (ch->aspd < 0)
                    ch->aspd = 0;
            }
        }

        ch->aorgspd = ch->aspd;
        setspd(ch);
    }
}

static void s_toneslide(chn_t *ch)
{
    if (volslidetype == 2) /* we came from an Lxy (toneslide+volslide) */
    {
        ch->info = ch->alasteff1;
    }
    else
    {
        if (ch->aorgspd == 0)
        {
            if (ch->asldspd == 0)
                return;

            ch->aorgspd = ch->asldspd;
            ch->aspd    = ch->asldspd;
        }

        if (ch->info == 0)
            ch->info = ch->alasteff1;
        else
            ch->alasteff1 = ch->info;
   }

    if (ch->aorgspd != ch->asldspd)
    {
        if (ch->aorgspd < ch->asldspd)
        {
            ch->aorgspd += (ch->info << 2);
            if ((uint16_t)(ch->aorgspd) > (uint16_t)(ch->asldspd))
                ch->aorgspd = ch->asldspd;
        }
        else
        {
            ch->aorgspd -= (ch->info << 2);
            if (ch->aorgspd < ch->asldspd)
                ch->aorgspd = ch->asldspd;
        }

        if (ch->aglis)
            ch->aspd = roundspd(ch, ch->aorgspd);
        else
            ch->aspd = ch->aorgspd;

        setspd(ch);
    }
}

static void s_vibrato(chn_t *ch)
{
    int8_t type;
    int16_t cnt;
    int32_t dat;

    if (volslidetype == 1) /* we came from a Kxy (vibrato+volslide) */
    {
        ch->info = ch->alasteff;
    }
    else
    {
        if (ch->info == 0)
            ch->info = ch->alasteff;

        if ((ch->info & 0xF0) == 0)
            ch->info = (ch->alasteff & 0xF0) | (ch->info & 0x0F);

        ch->alasteff = ch->info;
    }

    if (ch->aorgspd > 0)
    {
        cnt  = ch->avibcnt;
        type = (ch->avibtretype & 0x0E) >> 1;
        dat  = 0;

        /* sine */
        if ((type == 0) || (type == 4))
        {
            if (type == 4)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibsin[cnt >> 1];
        }

        /* ramp */
        else if ((type == 1) || (type == 5))
        {
            if (type == 5)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibramp[cnt >> 1];
        }

        /* square */
        else if ((type == 2) || (type == 6))
        {
            if (type == 6)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibsqu[cnt >> 1];
        }

        /* random */
        else if ((type == 3) || (type == 7))
        {
            if (type == 7)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat  = vibsin[cnt >> 1];
            cnt += (patmusicrand & 0x1E);
        }

        if (oldstvib)
            ch->aspd = ch->aorgspd + ((int16_t)(dat * (ch->info & 0x0F)) >> 4);
        else
            ch->aspd = ch->aorgspd + ((int16_t)(dat * (ch->info & 0x0F)) >> 5);

        setspd(ch);

        ch->avibcnt = (cnt + ((ch->info >> 4) << 1)) & 126;
    }
}

static void s_tremor(chn_t *ch)
{
    getlastnfo(ch);

    if (ch->atremor > 0)
    {
        ch->atremor--;
        return;
    }

    if (ch->atreon)
    {
        /* set to off */
        ch->atreon = false;

        ch->avol = 0;
        setvol(ch, true);

        ch->atremor = ch->info & 0x0F;
    }
    else
    {
        /* set to on */
        ch->atreon = true;

        ch->avol = ch->aorgvol;
        setvol(ch, true);

        ch->atremor = ch->info >> 4;
    }
}

static void s_arp(chn_t *ch)
{
    int8_t note, octa, noteadd;
    uint8_t tick;

    getlastnfo(ch);

    tick = musiccount % 3;

         if (tick == 1) noteadd = ch->info >> 4;
    else if (tick == 2) noteadd = ch->info & 0x0F;
    else                noteadd = 0;

    /* check for octave overflow */
    octa =  ch->lastnote & 0xF0;
    note = (ch->lastnote & 0x0F) + noteadd;

    while (note >= 12)
    {
        note -= 12;
        octa += 16;
    }

    ch->aspd = scalec2spd(ch, stnote2herz(octa | note));
    setspd(ch);
}

static void s_vibvol(chn_t *ch)
{
    volslidetype = 1;
    s_volslide(ch);
}

static void s_tonevol(chn_t *ch)
{
    volslidetype = 2;
    s_volslide(ch);
}

static void s_retrig(chn_t *ch)
{
    uint8_t infohi;

    getlastnfo(ch);
    infohi = ch->info >> 4;

    if (((ch->info & 0x0F) == 0) || (ch->atrigcnt < (ch->info & 0x0F)))
    {
        ch->atrigcnt++;
        return;
    }

    ch->atrigcnt = 0;

    voiceSetSamplePosition(ch->channelnum, 0);

    if (retrigvoladd[16 + infohi] == 0)
        ch->avol += retrigvoladd[infohi];
    else
        ch->avol = (int8_t)((ch->avol * retrigvoladd[16 + infohi]) >> 4);

    ch->avol = CLAMP(ch->avol, 0, 63);
    setvol(ch, true);

    ch->atrigcnt++; /* probably a bug? */
}

static void s_tremolo(chn_t *ch)
{
    int8_t type;
    int16_t cnt, dat;

    getlastnfo(ch);

    if ((ch->info & 0xF0) == 0)
        ch->info = (ch->alastnfo & 0xF0) | (ch->info & 0x0F);

    ch->alastnfo = ch->info;

    if (ch->aorgvol > 0)
    {
        cnt  = ch->avibcnt;
        type = ch->avibtretype >> 5;
        dat  = 0;

        /* sine */
        if ((type == 0) || (type == 4))
        {
            if (type == 4)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibsin[cnt >> 1];
        }

        /* ramp */
        else if ((type == 1) || (type == 5))
        {
            if (type == 5)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibramp[cnt >> 1];
        }

        /* square */
        else if ((type == 2) || (type == 6))
        {
            if (type == 6)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibsqu[cnt >> 1];
        }

        /* random */
        else if ((type == 3) || (type == 7))
        {
            if (type == 7)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat  = vibsin[cnt >> 1];
            cnt += (patmusicrand & 0x1E);
        }

        dat = ch->aorgvol + (int8_t)((dat * (ch->info & 0x0F)) >> 7);
        dat = CLAMP(dat, 0, 63);

        ch->avol = (int8_t)(dat);
        setvol(ch, true);

        ch->avibcnt = (cnt + ((ch->info & 0xF0) >> 3)) & 126;
    }
}

static void s_scommand1(chn_t *ch)
{
    getlastnfo(ch);
    ssoncejmp[ch->info >> 4](ch);
}

static void s_scommand2(chn_t *ch)
{
    getlastnfo(ch);
    ssotherjmp[ch->info >> 4](ch);
}

static void s_settempo(chn_t *ch)
{
    if (!musiccount && (ch->info >= 0x20))
        settempo(ch->info);
}

static void s_finevibrato(chn_t *ch)
{
    int8_t type;
    int16_t cnt;
    int32_t dat;

    if (ch->info == 0)
        ch->info = ch->alasteff;

    if ((ch->info & 0xF0) == 0)
        ch->info = (ch->alasteff & 0xF0) | (ch->info & 0x0F);

    ch->alasteff = ch->info;

    if (ch->aorgspd > 0)
    {
        cnt  =  ch->avibcnt;
        type = (ch->avibtretype & 0x0E) >> 1;
        dat  = 0;

        /* sine */
        if ((type == 0) || (type == 4))
        {
            if (type == 4)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibsin[cnt >> 1];
        }

        /* ramp */
        else if ((type == 1) || (type == 5))
        {
            if (type == 5)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibramp[cnt >> 1];
        }

        /* square */
        else if ((type == 2) || (type == 6))
        {
            if (type == 6)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat = vibsqu[cnt >> 1];
        }

        /* random */
        else if ((type == 3) || (type == 7))
        {
            if (type == 7)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80)
                    cnt = 0;
            }

            dat  = vibsin[cnt >> 1];
            cnt += (patmusicrand & 0x1E);
        }

        if (oldstvib)
            ch->aspd = ch->aorgspd + ((int16_t)(dat * (ch->info & 0x0F)) >> 6);
        else
            ch->aspd = ch->aorgspd + ((int16_t)(dat * (ch->info & 0x0F)) >> 7);

        setspd(ch);

        ch->avibcnt = (cnt + ((ch->info >> 4) << 1)) & 126;
    }
}

static void s_setgvol(chn_t *ch)
{
    if (ch->info <= 64)
        setglobalvol(ch->info);
}

static void voiceSetSource(uint8_t voiceNumber, const int8_t *sampleData,
    int32_t length, int32_t loopStart, int32_t loopLength,
    uint8_t loopFlag, uint8_t sampleIs16Bit)
{
    voice_t *v;

    v = &voice[voiceNumber];

    if (sampleData == NULL)
    {
        v->m_mixfunc = NULL; /* shut down voice */
        return;
    }

    v->m_loopflag = loopFlag ? true : false;
    v->m_end      = v->m_loopflag ? (loopStart + loopLength) : length;
    v->m_looplen  = loopLength;

    /* test sample swapping overflowing (with new sample length/loopLen) */
    if (v->m_pos >= v->m_end)
    {
        v->m_mixfunc = NULL;
        return;
    }

    if (sampleIs16Bit)
        v->m_base16 = (int16_t *)(sampleData);
    else
        v->m_base8 = sampleData;

    v->m_mixfunc = mixRoutineTable[(sampleIs16Bit << 2) + (interpolationFlag << 1) + v->m_loopflag];
}

static void voiceSetSamplePosition(uint8_t voiceNumber, uint16_t value)
{
    voice_t *v;

    v = &voice[voiceNumber];

    v->m_pos = value;
    v->m_posfrac = 0;

    /* confirmed ST3 overflow behavior */
    if (v->m_pos >= v->m_end)
    {
        if (soundcardtype == SOUNDCARD_SBPRO)
        {
            /* Sound Blaster */

            v->m_mixfunc = NULL; /* shut down voice */
        }
        else
        {
            /* GUS */

            if (v->m_loopflag)
            {
                /* loop wrapping */
                do
                {
                    v->m_pos -= v->m_looplen;
                }
                while (v->m_pos >= v->m_end);
            }
            else
            {
                v->m_mixfunc = NULL; /* shut down voice */
            }
        }
    }
}

static void voiceSetVolume(uint8_t voiceNumber, uint16_t vol, uint8_t pan)
{
    const uint16_t centerPanVal = 1 << ((4 - 1) + 6);
    uint16_t panL, panR, tmpPan;
    voice_t *v;
    chn_t *ch;

    v  = &voice[voiceNumber];
    ch = &chn[voiceNumber];

    if (!stereomode)
    {
        /* mono (center) */

        if (soundcardtype == SOUNDCARD_SBPRO)
        {
            panL = centerPanVal;
            panR = centerPanVal;
        }
        else
        {
            panL = guspantab[7];
            panR = guspantab[7];
        }
    }
    else
    {
        /* stereo */

        if (soundcardtype == SOUNDCARD_SBPRO)
        {
            tmpPan = pan << 4; /* 0..15 -> 0..960 */

            panL = 1024 - tmpPan;
            panR = tmpPan;
        }
        else
        {
            panL = guspantab[0x0F - pan];
            panR = guspantab[       pan];
        }
    }

    /* in SB stereo mode, you can use effect SAx to control certain things */
    if ((ch->amixtype > 0) && (soundcardtype == SOUNDCARD_SBPRO) && stereomode)
    {
        if (ch->amixtype >= 4)
        {
            /* center mixing */
            panL = centerPanVal;
            panR = centerPanVal;
        }
        else if ((ch->amixtype & 1) == 1)
        {
            /* swap L/R */
            tmpPan = panL;
            panL   = panR;
            panR   = tmpPan;
        }
    }

    v->m_vol_l = vol * panL;
    v->m_vol_r = vol * panR;
}

/* ----------------------------------------------------------------------- */
/*                          GENERAL MIXER MACROS                           */
/* ----------------------------------------------------------------------- */

#define GET_MIXER_VARS \
    audioMixL = mixBufferL; \
    audioMixR = mixBufferR; \
    mixInMono = (volL == volR); \
    realPos   = v->m_pos; \
    pos       = v->m_posfrac; /* 16.16 fixed point */ \
    delta     = v->m_speed; \

#define SET_BASE8 \
    base = v->m_base8; \
    smpPtr = base + realPos; \

#define SET_BASE16 \
    base = v->m_base16; \
    smpPtr = base + realPos; \

#define SET_BACK_MIXER_POS \
    v->m_posfrac = pos; \
    v->m_pos = realPos; \

#define GET_VOL \
    volL = v->m_vol_l; \
    volR = v->m_vol_r; \

#define INC_POS \
    pos += delta; \
    smpPtr += (pos >> 16); \
    pos &= 0xFFFF; \

/* ----------------------------------------------------------------------- */
/*                          SAMPLE RENDERING MACROS                        */
/* ----------------------------------------------------------------------- */

/* linear interpolation */
#define _LERP8(s1, s2, f) /* s1,s2 = -128..127 | f = 0..65535 (frac) */ \
    s2 -= s1; \
    s2 *= (f); \
    s2 >>= (16 - 8); \
    s1 <<= 8; \
    s1 += s2; \

#define _LERP16(s1, s2, f) /* s1,s2 = -32768..32767 | f = 0..65535 (frac) */  \
    s2 -= s1; \
    s2 >>= 1; \
    s2 *= (f); \
    s2 >>= (16 - 1); \
    s1 += s2; \

#define RENDER_8BIT_SMP \
    sample = *smpPtr; \
    *audioMixL++ += ((sample * volL) >> (16 - 8)); \
    *audioMixR++ += ((sample * volR) >> (16 - 8)); \

#define RENDER_8BIT_SMP_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP8(sample, sample2, pos) \
    *audioMixL++ += ((sample * volL) >> 16); \
    *audioMixR++ += ((sample * volR) >> 16); \

#define RENDER_16BIT_SMP \
    sample = *smpPtr; \
    *audioMixL++ += ((sample * volL) >> 16); \
    *audioMixR++ += ((sample * volR) >> 16); \

#define RENDER_16BIT_SMP_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP16(sample, sample2, pos) \
    *audioMixL++ += ((sample * volL) >> 16); \
    *audioMixR++ += ((sample * volR) >> 16); \

#define RENDER_8BIT_SMP_MONO \
    sample = ((*smpPtr * volL) >> (16 - 8)); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_8BIT_SMP_MONO_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP8(sample, sample2, pos) \
    sample = ((sample * volL) >> 16); \
    \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_16BIT_SMP_MONO \
    sample = ((*smpPtr * volL) >> 16); \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

#define RENDER_16BIT_SMP_MONO_LERP \
    sample  = *(smpPtr    ); \
    sample2 = *(smpPtr + 1); \
    _LERP16(sample, sample2, pos) \
    sample = ((sample * volL) >> 16); \
    \
    *audioMixL++ += sample; \
    *audioMixR++ += sample; \

/* ----------------------------------------------------------------------- */
/*                     "SAMPLES TO MIX" LIMITING MACROS                    */
/* ----------------------------------------------------------------------- */

#define LIMIT_MIX_NUM \
    limited = true; \
    \
    i = (v->m_end - realPos) - 1; \
    if (v->m_speed > (i >> 16)) \
    { \
        if (i > 65535) /* won't fit in a 32-bit div */ \
        { \
            samplesToMix = ((uint32_t)(pos ^ 0xFFFFFFFF) / v->m_speed) + 1; \
            limited = false; \
        } \
        else \
        { \
            samplesToMix = ((uint32_t)((i << 16) | (pos ^ 0x0000FFFF)) / v->m_speed) + 1; \
        } \
    } \
    else \
    { \
        samplesToMix = 65535; \
    } \
    \
    if (samplesToMix > (uint32_t)(samplesToRender)) \
    { \
        samplesToMix = samplesToRender; \
        limited = false; \
    } \
    \
    samplesToRender -= samplesToMix; \

/* ----------------------------------------------------------------------- */
/*                     SAMPLE END/LOOP WRAPPING MACROS                     */
/* ----------------------------------------------------------------------- */

#define HANDLE_SAMPLE_END \
    realPos = (uint32_t)(smpPtr - base); \
    if (limited) \
    { \
        v->m_mixfunc = NULL; /* shut down voice */ \
        return; \
    } \

#define WRAP_LOOP \
    realPos = (uint32_t)(smpPtr - base); \
    if (limited) \
    { \
        do \
        { \
            realPos -= v->m_looplen; \
        } \
        while (realPos >= v->m_end); \
        \
        smpPtr = base + realPos; \
    } \

/* ----------------------------------------------------------------------- */
/*                       VOLUME=0 OPTIMIZATION MACROS                      */
/* ----------------------------------------------------------------------- */

#define VOL0_OPTIMIZATION_NO_LOOP \
    realPos = v->m_pos + ((v->m_speed >>    16) * numSamples); \
    pos = v->m_posfrac + ((v->m_speed & 0xFFFF) * numSamples); \
    \
    realPos += (pos >> 16); \
    pos &= 0xFFFF; \
    \
    if (realPos >= v->m_end) \
    { \
        v->m_mixfunc = NULL; \
        return; \
    } \
    \
    SET_BACK_MIXER_POS \

#define VOL0_OPTIMIZATION_LOOP \
    realPos = v->m_pos + ((v->m_speed >>    16) * numSamples); \
    pos = v->m_posfrac + ((v->m_speed & 0xFFFF) * numSamples); \
    \
    realPos += (pos >> 16); \
    pos &= 0xFFFF; \
    \
    while (realPos >= v->m_end) \
           realPos -= v->m_looplen; \
    \
    SET_BACK_MIXER_POS \

/* ----------------------------------------------------------------------- */
/*                          8-BIT MIXING ROUTINES                          */
/* ----------------------------------------------------------------------- */

static void mix8bNoLoop(voice_t *v, uint32_t numSamples)
{
    const int8_t *base;
    uint8_t mixInMono, limited;
    int32_t sample, *audioMixL, *audioMixR, samplesToRender;
    register const int8_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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
    const int8_t *base;
    uint8_t mixInMono, limited;
    int32_t sample, *audioMixL, *audioMixR, samplesToRender;
    register const int8_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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

static void mix8bNoLoopLerp(voice_t *v, uint32_t numSamples)
{
    const int8_t *base;
    uint8_t mixInMono, limited;
    int32_t sample, sample2, *audioMixL, *audioMixR, samplesToRender;
    register const int8_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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
    const int8_t *base;
    uint8_t mixInMono, limited;
    int32_t sample, sample2, *audioMixL, *audioMixR, samplesToRender;
    register const int8_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE8

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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

/* ----------------------------------------------------------------------- */
/*                          16-BIT MIXING ROUTINES                         */
/* ----------------------------------------------------------------------- */

static void mix16bNoLoop(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, limited;
    const int16_t *base;
    int32_t sample, *audioMixL, *audioMixR, samplesToRender;
    register const int16_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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
    uint8_t mixInMono, limited;
    const int16_t *base;
    int32_t sample, *audioMixL, *audioMixR, samplesToRender;
    register const int16_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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

static void mix16bNoLoopLerp(voice_t *v, uint32_t numSamples)
{
    uint8_t mixInMono, limited;
    const int16_t *base;
    int32_t sample, sample2, *audioMixL, *audioMixR, samplesToRender;
    register const int16_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_NO_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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
    uint8_t mixInMono, limited;
    const int16_t *base;
    int32_t sample, sample2, *audioMixL, *audioMixR, samplesToRender;
    register const int16_t *smpPtr;
    register int32_t volL, volR;
    register uint32_t pos, delta;
    uint32_t realPos, i, samplesToMix;

    GET_VOL
    if ((volL == 0) && (volR == 0))
    {
        VOL0_OPTIMIZATION_LOOP
        return;
    }

    GET_MIXER_VARS
    SET_BASE16

    samplesToRender = numSamples;
    while (samplesToRender > 0)
    {
        LIMIT_MIX_NUM
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

mixRoutine mixRoutineTable[8] =
{
    (mixRoutine)(mix8bNoLoop),
    (mixRoutine)(mix8bLoop),
    (mixRoutine)(mix8bNoLoopLerp),
    (mixRoutine)(mix8bLoopLerp),
    (mixRoutine)(mix16bNoLoop),
    (mixRoutine)(mix16bLoop),
    (mixRoutine)(mix16bNoLoopLerp),
    (mixRoutine)(mix16bLoopLerp)
};

/* ----------------------------------------------------------------------- */

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
    for (i = 0; i < 32; ++i)
    {
        v = &voice[i];

        /* call the mixing routine currently set for the voice */
        if (v->m_mixfunc != NULL)
            (v->m_mixfunc)((void *)(v), sampleBlockLength);
    }

    if (mastervol == 256)
    {
        /* user-adjustable volume is at max */
        for (i = 0; i < sampleBlockLength; ++i)
        {
            /* left channel */
            out32 = (mixBufferL[i] * mastermul) >> 8;
            CLAMP16(out32); /* FAST 16-bit clamp technique */
            *stream++ = (int16_t)(out32);

            /* right channel */
            out32 = (mixBufferR[i] * mastermul) >> 8;
            CLAMP16(out32);
            *stream++ = (int16_t)(out32);
        }
    }
    else
    {
        /* user-adjustable volume is not at max, adjust amplitude */
        for (i = 0; i < sampleBlockLength; ++i)
        {
            /* left channel */
            out32 = (mixBufferL[i] * mastermul) >> 8;
            CLAMP16(out32); /* FAST 16-bit clamp technique */
            out32 = (out32 * mastervol) >> 8; /* user-adjustable volume */
            *stream++ = (int16_t)(out32);

            /* right channel */
            out32 = (mixBufferR[i] * mastermul) >> 8;
            CLAMP16(out32);
            out32 = (out32 * mastervol) >> 8;
            *stream++ = (int16_t)(out32);
        }
    }
}

static void st3play_FillAudioBuffer(int16_t *buffer, int32_t samples)
{
    int32_t a, b;

    a = samples;
    while (a > 0)
    {
        if (samplesLeft == 0)
        {
            /* new replayer tick */
            if (!musicPaused)
                dorow();

            samplesLeft = samplesPerTick;
        }

        b = a;
        if (b > samplesLeft)
            b = samplesLeft;

        mixAudio(buffer, b);
        buffer += (b * sizeof (int16_t));

        a -= b;
        samplesLeft -= b;
    }

    sampleCounter += samples;
}

void st3play_Close(void)
{
    uint8_t i;

    closeMixer();

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

    for (i = 0; i < 100; ++i)
    {
        if (ins[i].data != NULL)
        {
            free(ins[i].data);
            ins[i].data = NULL;
        }

        if (patdata[i] != NULL)
        {
            free(patdata[i]);
            patdata[i]  = NULL;
        }
    }
}

void st3play_PauseSong(int8_t flag)
{
    musicPaused = flag ? true : false;
}

void st3play_TogglePause(void)
{
    musicPaused ^= 1;
}

void st3play_SetMasterVol(uint16_t vol)
{
    mastervol = CLAMP(vol, 0, 256);
}

void st3play_SetInterpolation(uint8_t flag)
{
    int32_t i;

    interpolationFlag = flag ? true : false;

    /* shut down voices to prevent mixture of interpolated/non-interpolated voices */
    for (i = 0; i < 32; ++i)
        voice[i].m_mixfunc = NULL;
}

char *st3play_GetSongName(void)
{
    return (songname);
}

uint32_t st3play_GetMixerTicks(void)
{
    if (audioRate < 1000)
        return (0);

    return (sampleCounter / (audioRate / 1000));
}

/* adds wrapped sample after loop/end (for branchless linear interpolation) */
static void fixSample(int8_t *data, uint8_t loopFlag, uint8_t sixteenBit, int32_t length, int32_t loopStart, int32_t loopLength)
{
    int16_t *ptr16;

    ptr16 = (int16_t *)(data);
    if (loopFlag)
    {
        /* loop */
        if (sixteenBit)
            ptr16[loopStart + loopLength] = ptr16[loopStart];
        else
            data[loopStart + loopLength] = data[loopStart];
    }
    else
    {
        /* no loop */
        if (sixteenBit)
            ptr16[length] = 0;
        else
            data[length] = 0;
    }
}

static int8_t loadS3M(const uint8_t *dat, uint32_t modLen)
{
    uint8_t pan, *ptr8, signedSamples;
    int16_t k, *smpReadPtr16, *smpWritePtr16;
    uint16_t patDataLen;
    uint32_t i, j, offs, loopEnd;

    if ((modLen < 0x70) || (dat[0x1C] != 0x1A) || (dat[0x1D] != 16) || (memcmp(&dat[0x2C], "SCRM", 4) != 0))
        return (false); /* not a valid S3M */

    memcpy(songname, dat, 28);
    songname[28] = '\0';

    signedSamples = (*((uint16_t *)(&dat[0x2A])) == 1);

    ordNum = *((uint16_t *)(&dat[0x20])); if (ordNum > 256) ordNum = 256;
    insNum = *((uint16_t *)(&dat[0x22])); if (insNum > 100) insNum = 100;
    patNum = *((uint16_t *)(&dat[0x24])); if (patNum > 100) patNum = 100;

    memcpy(order,       &dat[0x60], ordNum);
    memcpy(chnsettings, &dat[0x40], 32);

    /* load instrument headers */
    memset(ins, 0, sizeof (ins));
    for (i = 0; i < insNum; ++i)
    {
        offs = (*((uint16_t *)(&dat[0x60 + ordNum + (i * 2)]))) << 4;
        if (offs == 0)
            continue; /* empty */

        ptr8 = (uint8_t *)(&dat[offs]);

        ins[i].type    = ptr8[0x00];
        ins[i].length  = *((uint32_t *)(&ptr8[0x10]));
        ins[i].loopbeg = *((uint32_t *)(&ptr8[0x14]));
        loopEnd        = *((uint32_t *)(&ptr8[0x18]));
        ins[i].vol     = CLAMP((int8_t)(ptr8[0x1C]), 0, 63); /* ST3 clamps smp vol to 63, to prevent mix vol overflow */
        ins[i].flags   = ptr8[0x1F];
        ins[i].c2spd   = *((uint16_t *)(&ptr8[0x20])); /* ST3 only reads the lower word of this one */

        /* reduce sample length if it overflows the module size (f.ex. "miracle man.s3m") */
        offs = ((ptr8[0x0D] << 16) | (ptr8[0x0F] << 8) | ptr8[0x0E]) << 4;
        if ((offs + ins[i].length) >= modLen)
            ins[i].length = modLen - offs;

        if (loopEnd < ins[i].loopbeg)
            loopEnd = ins[i].loopbeg + 1;

        if ((ins[i].loopbeg >= ins[i].length) || (loopEnd > ins[i].length))
            ins[i].flags &= 0xFE; /* turn off loop */

        ins[i].looplen = loopEnd - ins[i].loopbeg;
        if ((ins[i].looplen == 0) || ((ins[i].loopbeg + ins[i].looplen) > ins[i].length))
            ins[i].flags &= 0xFE; /* turn off loop */
    }

    /* load pattern data */
    memset(patdata, 0, sizeof (patdata));
    for (i = 0; i < patNum; ++i)
    {
        offs = (*((uint16_t *)(&dat[0x60 + ordNum + (insNum * 2) + (i * 2)]))) << 4;
        if (offs == 0)
            continue; /* empty */

        patDataLen = *((uint16_t *)(&dat[offs]));
        if (patDataLen > 0)
        {
            patdata[i] = (uint8_t *)(malloc(patDataLen));
            if (patdata[i] == NULL)
            {
                st3play_Close();
                return (false);
            }

            memcpy(patdata[i], &dat[offs + 2], patDataLen);
        }
    }

    /* load sample data */
    for (i = 0; i < insNum; ++i)
    {
        offs = (*((uint16_t *)(&dat[0x60 + ordNum + (i * 2)]))) << 4;
        if (offs == 0)
            continue; /* empty */

        if ((ins[i].length <= 0) || (ins[i].type != 1) || (dat[offs + 0x1E] != 0))
            continue; /* sample not supported */

        offs = ((dat[offs + 0x0D] << 16) | (dat[offs + 0x0F] << 8) | dat[offs + 0x0E]) << 4;
        if (offs == 0)
            continue; /* empty */

        /* offs now points to sample data */

        if (ins[i].flags & 4) /* 16-bit */
            ins[i].data = (int8_t *)(malloc((ins[i].length * 2) + 2));
        else
            ins[i].data = (int8_t *)(malloc(ins[i].length + 1));

        if (ins[i].data == NULL)
        {
            st3play_Close();
            return (false);
        }

        if (ins[i].flags & 4)
        {
            /* 16-bit */
            if (signedSamples)
            {
                memcpy(ins[i].data, &dat[offs], ins[i].length * 2);
            }
            else
            {
                smpReadPtr16  = (int16_t *)(&dat[offs]);
                smpWritePtr16 = (int16_t *)(ins[i].data);

                for (j = 0; j < ins[i].length; ++j)
                    smpWritePtr16[j] = smpReadPtr16[j] + 32768;
            }
        }
        else
        {
            /* 8-bit */
            if (signedSamples)
            {
                memcpy(ins[i].data, &dat[offs], ins[i].length);
            }
            else
            {
                for (j = 0; j < ins[i].length; ++j)
                    ins[i].data[j] = dat[offs + j] + 128;
            }
        }

        /* fix sample data for branchless linear interpolation */
        fixSample(ins[i].data, ins[i].flags & 1, ins[i].flags & 4, ins[i].length, ins[i].loopbeg, ins[i].looplen);
    }

    /* set up pans */
    for (i = 0; i < 32; ++i)
    {
        chn[i].apanpos = 0x7;
        if (chnsettings[i] != 0xFF)
            chn[i].apanpos = (chnsettings[i] & 8) ? 0xC : 0x3;

        if ((soundcardtype == SOUNDCARD_GUS) && (dat[0x35] == 252))
        {
            /* non-default pannings */
            pan = dat[0x60 + ordNum + (insNum * 2) + (patNum * 2) + i];
            if (pan & 32)
                chn[i].apanpos = pan & 0x0F;
        }
    }

    /* count real amount of orders */
    for (k = (ordNum - 1); k >= 0; --k)
    {
        if (dat[0x60 + k] != 255)
            break;
    }
    ordNum = k;

    setspeed(6);
    settempo(125);
    setglobalvol(64);

    amigalimits  = (dat[0x26] & 0x10) ? true : false;
    oldstvib     =  dat[0x26] & 0x01;
    mastermul    =  dat[0x33];
    fastvolslide = ((*((uint16_t *)(&dat[0x28])) == 0x1300) || (dat[0x26] & 0x40)) ? true : false;

    if (signedSamples)
    {
        switch (mastermul)
        {
            case 0: mastermul = 0x10; break;
            case 1: mastermul = 0x20; break;
            case 2: mastermul = 0x30; break;
            case 3: mastermul = 0x40; break;
            case 4: mastermul = 0x50; break;
            case 5: mastermul = 0x60; break;
            case 6: mastermul = 0x70; break;
            case 7: mastermul = 0x7F; break;
            default:                  break;
        }
    }

    /* taken from the ST3.21 loader, strange stuff... */
    if (mastermul == 2)        mastermul = 0x20;
    if (mastermul == (2 + 16)) mastermul = 0x20 + 128;

    stereomode = mastermul & 128;

    mastermul &= 127;
    if (mastermul == 0)
        mastermul = 48; /* default in ST3 when you play a song where mastermul=0 */

    mastermul *= 2; /* upscale for st3play's audio mixer */

    if (dat[0x32] > 0)    settempo(dat[0x32]);
    if (dat[0x30] != 255) setglobalvol(dat[0x30]);

    if ((dat[0x31] > 0) && (dat[0x31] != 255))
        setspeed(dat[0x31]);

    if (amigalimits)
    {
        aspdmin =  907 / 2;
        aspdmax = 1712 * 2;
    }
    else
    {
        aspdmin = 64;
        aspdmax = 32767;
    }

    for (i = 0; i < 32; ++i)
    {
        chn[i].channelnum   = (int8_t)(i);
        chn[i].achannelused = 0x80;
    }

    np_patseg    = NULL;
    musiccount   = 0;
    patterndelay = 0;
    patloopcount = 0;
    startrow     = 0;
    breakpat     = 0;
    volslidetype = 0;
    np_patoff    = -1;
    jmptoord     = -1;

    np_ord = 0;
    neworder();

    lastachannelused = 1;
    return (true);
}

int8_t st3play_PlaySong(const uint8_t *moduleData, uint32_t dataLength, uint8_t useInterpolationFlag, uint8_t soundCardType, uint32_t audioFreq)
{
    st3play_Close();
    memset(songname, 0, sizeof (songname));

    if (audioFreq == 0)
        audioFreq = 44100;

    audioFreq = CLAMP(audioFreq, 11025, 65535); /* st3play can't do higher rates than 65535 (setspd() calc.) */

    sampleCounter     = 0;
    soundcardtype     = (soundCardType != SOUNDCARD_GUS) ? SOUNDCARD_SBPRO : SOUNDCARD_GUS;
    musicPaused       = true;
    audioRate         = audioFreq;
    soundBufferSize   = MIX_BUF_SAMPLES;
    interpolationFlag = useInterpolationFlag ? true : false;

    memset(chn,   0, sizeof (chn));
    memset(voice, 0, sizeof (voice));

    mixBufferL = (int32_t *)(malloc(MIX_BUF_SAMPLES * sizeof (int32_t)));
    mixBufferR = (int32_t *)(malloc(MIX_BUF_SAMPLES * sizeof (int32_t)));

    if ((mixBufferL == NULL) || (mixBufferR == NULL))
    {
        st3play_Close();
        return (false);
    }

    if (!openMixer(audioRate))
    {
        st3play_Close();
        return (false);
    }

    if (!loadS3M(moduleData, dataLength))
    {
        st3play_Close();
        return (false);
    }

    musicPaused = false;
    return (true);
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
        st3play_FillAudioBuffer((int16_t *)(waveBlock->lpData), MIX_BUF_SAMPLES);
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

    samplesLeft = 0;
    currBuffer  = 0;

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
