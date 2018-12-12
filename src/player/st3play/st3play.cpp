#include "player/st3play/st3play.h"
#include <cstddef>
#include <cstring>

//
// ST3PLAY v0.88 - 28th of November 2018 - https://16-bits.org
// ===========================================================
// Very accurate C port of Scream Tracker 3.21's replayer,
// by Olav "8bitbubsy" Sørensen. Using the original asm source codes
// by Sami "PSI" Tammilehto (Future Crew) with permission.
//

// Modified by Claudio Matsuoka, Nov 2018
// - Port to C++
// - Changes to use new mixer subsystem
// - Changes to be big-endian safe
// - Add mod file importer (to play ST3 M.K. files correctly)

namespace {

enum {
    PATT_SEP = 254,
    PATT_END = 255,
};

/* STRUCTS */

//typedef void (*mixRoutine)(void *, int32_t);


typedef void (St3Play::*effect_routine)(chn_t *ch);

/* 8bitbubsy: this panning table was made sampling audio from my GUS PnP and processing it.
** It was scaled from 0..1 to 0..960 to fit the mixing volumes used for SB Pro mixing (4-bit pan * 16) */
static const int16_t guspantab[16] = { 0, 245, 365, 438, 490, 562, 628, 682, 732, 827, 775, 847, 879, 909, 935, 960 };

/* TABLES */
const int8_t retrigvoladd[32] = {
    0, -1, -2, -4, -8,-16,  0,  0,
    0,  1,  2,  4,  8, 16,  0,  0,
    0,  0,  0,  0,  0,  0, 10,  8,
    0,  0,  0,  0,  0,  0, 24, 32
};

const uint8_t octavediv[16] = {
    0, 1, 2, 3, 4, 5, 6, 7,

    /* overflow data from xvol_amiga table */
    0, 5, 11, 17, 27, 32, 42, 47
};

const int16_t notespd[16] = {
    1712 * 16, 1616 * 16, 1524 * 16,
    1440 * 16, 1356 * 16, 1280 * 16,
    1208 * 16, 1140 * 16, 1076 * 16,
    1016 * 16,  960 * 16,  907 * 16,
    1712 * 8,

    /* overflow data from adlibiadd table */
    0x0100, 0x0802, 0x0A09
};

const int16_t vibsin[64] = {
     0x00, 0x18, 0x31, 0x4A, 0x61, 0x78, 0x8D, 0xA1,
     0xB4, 0xC5, 0xD4, 0xE0, 0xEB, 0xF4, 0xFA, 0xFD,
     0xFF, 0xFD, 0xFA, 0xF4, 0xEB, 0xE0, 0xD4, 0xC5,
     0xB4, 0xA1, 0x8D, 0x78, 0x61, 0x4A, 0x31, 0x18,
     0x00,-0x18,-0x31,-0x4A,-0x61,-0x78,-0x8D,-0xA1,
    -0xB4,-0xC5,-0xD4,-0xE0,-0xEB,-0xF4,-0xFA,-0xFD,
    -0xFF,-0xFD,-0xFA,-0xF4,-0xEB,-0xE0,-0xD4,-0xC5,
    -0xB4,-0xA1,-0x8D,-0x78,-0x61,-0x4A,-0x31,-0x18
};

const uint8_t vibsqu[64] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const int16_t vibramp[64] = {
       0, -248,-240,-232,-224,-216,-208,-200,
    -192, -184,-176,-168,-160,-152,-144,-136,
    -128, -120,-112,-104, -96, -88, -80, -72,
     -64,  -56, -48, -40, -32, -24, -16,  -8,
       0,    8,  16,  24,  32,  40,  48,  56,
      64,   72,  80,  88,  96, 104, 112, 120,
     128,  136, 144, 152, 160, 168, 176, 184,
     192,  200, 208, 216, 224, 232, 240, 248
};

effect_routine ssoncejmp[16] = {
    &St3Play::s_ret,         // 0
    &St3Play::s_setgliss,    // 1
    &St3Play::s_setfinetune, // 2
    &St3Play::s_setvibwave,  // 3
    &St3Play::s_settrewave,  // 4
    &St3Play::s_ret,         // 5
    &St3Play::s_ret,         // 6
    &St3Play::s_ret,         // 7
    &St3Play::s_setpanpos,   // 8
    &St3Play::s_ret,         // 9
    &St3Play::s_stereocntr,  // A
    &St3Play::s_patloop,     // B
    &St3Play::s_notecut,     // C
    &St3Play::s_notedelay,   // D
    &St3Play::s_patterdelay, // E
    &St3Play::s_ret          // F
};

effect_routine ssotherjmp[16] = {
    &St3Play::s_ret,        // 0
    &St3Play::s_ret,        // 1
    &St3Play::s_ret,        // 2
    &St3Play::s_ret,        // 3
    &St3Play::s_ret,        // 4
    &St3Play::s_ret,        // 5
    &St3Play::s_ret,        // 6
    &St3Play::s_ret,        // 7
    &St3Play::s_ret,        // 8
    &St3Play::s_ret,        // 9
    &St3Play::s_ret,        // A
    &St3Play::s_ret,        // B
    &St3Play::s_notecutb,   // C
    &St3Play::s_notedelayb, // D
    &St3Play::s_ret,        // E
    &St3Play::s_ret         // F
};

effect_routine soncejmp[27] = {
    &St3Play::s_ret,       // .
    &St3Play::s_setspeed,  // A
    &St3Play::s_jmpto,     // B
    &St3Play::s_break,     // C
    &St3Play::s_volslide,  // D
    &St3Play::s_slidedown, // E
    &St3Play::s_slideup,   // F
    &St3Play::s_ret,       // G
    &St3Play::s_ret,       // H
    &St3Play::s_tremor,    // I
    &St3Play::s_arp,       // J
    &St3Play::s_ret,       // K
    &St3Play::s_ret,       // L
    &St3Play::s_ret,       // M
    &St3Play::s_ret,       // N
    &St3Play::s_ret,       // O - handled in doamiga()
    &St3Play::s_ret,       // P
    &St3Play::s_retrig,    // Q
    &St3Play::s_ret,       // R
    &St3Play::s_scommand1, // S
    &St3Play::s_settempo,  // T
    &St3Play::s_ret,       // U
    &St3Play::s_ret,       // V
    &St3Play::s_ret,       // W
    &St3Play::s_ret,       // X
    &St3Play::s_ret,       // Y
    &St3Play::s_ret        // Z
};

effect_routine sotherjmp[27] = {
    &St3Play::s_ret,         // .
    &St3Play::s_ret,         // A
    &St3Play::s_ret,         // B
    &St3Play::s_ret,         // C
    &St3Play::s_volslide,    // D
    &St3Play::s_slidedown,   // E
    &St3Play::s_slideup,     // F
    &St3Play::s_toneslide,   // G
    &St3Play::s_vibrato,     // H
    &St3Play::s_tremor,      // I
    &St3Play::s_arp,         // J
    &St3Play::s_vibvol,      // K
    &St3Play::s_tonevol,     // L
    &St3Play::s_ret,         // M
    &St3Play::s_ret,         // N
    &St3Play::s_ret,         // O
    &St3Play::s_ret,         // P
    &St3Play::s_retrig,      // Q
    &St3Play::s_tremolo,     // R
    &St3Play::s_scommand2,   // S
    &St3Play::s_ret,         // T
    &St3Play::s_finevibrato, // U
    &St3Play::s_setgvol,     // V
    &St3Play::s_ret,         // W
    &St3Play::s_ret,         // X
    &St3Play::s_ret,         // Y
    &St3Play::s_ret          // Z
};

}  // namespace


St3Play::St3Play() :
    inside_loop_(false)
{
    memset(chn, 0, sizeof(chn));
    memset(voice, 0, sizeof(voice));
}

St3Play::~St3Play()
{
    if (!patterns_in_place) {
        for (int i = 0; i < patNum; i++) {
            delete [] patdata[i];
        }
    }

    if (!instruments_in_place) {
        for (int i = 0; i < insNum; i++) {
            delete [] ins[i].data;
        }
    }
}


/* CODE START */

void St3Play::getlastnfo(chn_t *ch)
{
    if (ch->info == 0)
        ch->info = ch->alastnfo;
}

void St3Play::setspeed(uint8_t val)
{
    if (val > 0)
        musicmax = val;
}

void St3Play::settempo(uint8_t val)
{
    if (val > 32) {
        samplesPerTick = (audioRate * 125) / (50 * val);
        tempo_ = val;
    }
}

void St3Play::setspd(chn_t *ch)
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

    if (tmpspd > 0) {
        mixer_->set_period(ch->channelnum, double(tmpspd) / 4.0);
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

void St3Play::setglobalvol(int8_t vol)
{
    globalvol = vol;

    if ((uint8_t)(vol) > 64)
        vol = 64;

    useglobalvol = globalvol * 4; /* for mixer */
}

void St3Play::setvol(chn_t *ch, uint8_t volFlag)
{
    if (volFlag)
        ch->achannelused |= 0x80;

    voiceSetVolume(ch->channelnum, (ch->avol * useglobalvol) >> 8, ch->apanpos & 0x0F);
}

int16_t St3Play::stnote2herz(uint8_t note)
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

int16_t St3Play::scalec2spd(chn_t *ch, int16_t spd)
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
int16_t St3Play::roundspd(chn_t *ch, int16_t spd)
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

int16_t St3Play::neworder() /* rewritten to be more safe */
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

int8_t St3Play::getnote1()
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

void St3Play::doamiga(uint8_t channel)
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
                    if ((soundcardtype == SOUNDCARD_SB) || ((ch->cmd != ('G' - 64)) && (ch->cmd != ('L' - 64))))
                    {
                        /* on GUS, do no sample swapping without a note number */
                        if ((soundcardtype != SOUNDCARD_GUS) || (ch->note != 255))
                        {
                            voiceSetSource(channel, /*(const int8_t *)(ins[smp].data)*/ smp, ins[smp].length, ins[smp].loopbeg,
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

void St3Play::donewnote(uint8_t channel, int8_t notedelayflag)
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

void St3Play::donotes()
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
void St3Play::docmd1() /* what a mess... */
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
                    (*this.*soncejmp[ch->cmd])(ch);
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
                    (*this.*soncejmp[ch->cmd])(ch);
                }
            }
        }
    }
}

void St3Play::docmd2() /* tick >0 commands */
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
                (*this.*sotherjmp[ch->cmd])(ch);
            }
        }
    }
}

void St3Play::dorow()
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

void St3Play::s_ret(chn_t *ch)
{
    (void)(ch);
}

void St3Play::s_setgliss(chn_t *ch)
{
    ch->aglis = ch->info & 0x0F;
}

void St3Play::s_setfinetune(chn_t *ch)
{
    /* this has a bug in ST3 that makes this effect do nothing! */
    (void)(ch);
}

void St3Play::s_setvibwave(chn_t *ch)
{
    ch->avibtretype = (ch->avibtretype & 0xF0) | ((ch->info << 1) & 0x0F);
}

void St3Play::s_settrewave(chn_t *ch)
{
    ch->avibtretype = ((ch->info << 5) & 0xF0) | (ch->avibtretype & 0x0F);
}

void St3Play::s_setpanpos(chn_t *ch)
{
    if (soundcardtype == SOUNDCARD_GUS)
    {
        ch->apanpos = ch->info & 0x0F;
        setvol(ch, false);
    }
}

void St3Play::s_stereocntr(chn_t *ch)
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

    if (soundcardtype == SOUNDCARD_SB)
    {
        if ((ch->info & 0x0F) < 8)
        {
            ch->amixtype = ch->info & 0x0F;
            setvol(ch, false);
        }
    }
}

void St3Play::s_patloop(chn_t *ch)
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
        inside_loop_ = true;
    }
    else
    {
        patloopcount = 0;
        patloopstart = np_row + 1;
        inside_loop_ = false;
    }
}

void St3Play::s_notecut(chn_t *ch)
{
    ch->anotecutcnt = ch->info & 0x0F;
}

void St3Play::s_notecutb(chn_t *ch)
{
    if (ch->anotecutcnt > 0)
    {
        ch->anotecutcnt--;
        if (ch->anotecutcnt == 0)
            voice[ch->channelnum].m_speed = 0; /* shut down voice (recoverable by using pitch effects) */
    }
}

void St3Play::s_notedelay(chn_t *ch)
{
    ch->anotedelaycnt = ch->info & 0x0F;
}

void St3Play::s_notedelayb(chn_t *ch)
{
    if (ch->anotedelaycnt > 0)
    {
        ch->anotedelaycnt--;
        if (ch->anotedelaycnt == 0)
            donewnote(ch->channelnum, true);
    }
}

void St3Play::s_patterdelay(chn_t *ch)
{
    if (patterndelay == 0)
        patterndelay = ch->info & 0x0F;
}

void St3Play::s_setspeed(chn_t *ch)
{
    setspeed(ch->info);
}

void St3Play::s_jmpto(chn_t *ch)
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

void St3Play::s_break(chn_t *ch)
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

void St3Play::s_volslide(chn_t *ch)
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

void St3Play::s_slidedown(chn_t *ch)
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

void St3Play::s_slideup(chn_t *ch)
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

void St3Play::s_toneslide(chn_t *ch)
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

void St3Play::s_vibrato(chn_t *ch)
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

void St3Play::s_tremor(chn_t *ch)
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

void St3Play::s_arp(chn_t *ch)
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

void St3Play::s_vibvol(chn_t *ch)
{
    volslidetype = 1;
    s_volslide(ch);
}

void St3Play::s_tonevol(chn_t *ch)
{
    volslidetype = 2;
    s_volslide(ch);
}

void St3Play::s_retrig(chn_t *ch)
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

void St3Play::s_tremolo(chn_t *ch)
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

void St3Play::s_scommand1(chn_t *ch)
{
    getlastnfo(ch);
    (*this.*ssoncejmp[ch->info >> 4])(ch);
}

void St3Play::s_scommand2(chn_t *ch)
{
    getlastnfo(ch);
    (*this.*ssotherjmp[ch->info >> 4])(ch);
}

void St3Play::s_settempo(chn_t *ch)
{
    if (!musiccount && (ch->info >= 0x20))
        settempo(ch->info);
}

void St3Play::s_finevibrato(chn_t *ch)
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

void St3Play::s_setgvol(chn_t *ch)
{
    if (ch->info <= 64)
        setglobalvol(ch->info);
}

// ---------------------------------------------------------------------------

void St3Play::voiceSetSource(uint8_t voiceNumber, int smp /*const int8_t *sampleData*/,
    int32_t length, int32_t loopStart, int32_t loopLength,
    uint8_t loopFlag, uint8_t sampleIs16Bit)
{
    mixer_->set_sample(voiceNumber, smp + 1);
    mixer_->set_loop_start(voiceNumber, loopStart);
    mixer_->set_loop_end(voiceNumber, loopStart + loopLength);
    mixer_->enable_loop(voiceNumber, loopFlag != 0);
}

void St3Play::voiceSetSamplePosition(uint8_t voiceNumber, uint16_t value)
{
    mixer_->set_voicepos(voiceNumber, value);
}

void St3Play::voiceSetVolume(uint8_t voiceNumber, uint16_t vol, uint8_t pan)
{
    mixer_->set_volume(voiceNumber, vol << 2);
    mixer_->set_pan(voiceNumber, int(pan << 4) - 0x80);
}

