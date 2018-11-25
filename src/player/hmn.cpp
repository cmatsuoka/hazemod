#include "player/hmn.h"
#include <cstring>

// His Master's Noise Replayer
//
// Based on Musicdisktrackerreplay Pex "Mahoney" Tufvesson (December1990).
//
// ## Notes:
//
// From http://www.livet.se/mahoney/:
//
// Most modules from His Master's Noise uses special chip-sounds or
// fine-tuning of samples that never was a part of the standard NoiseTracker
// v2.0 command set. So if you want to listen to them correctly use an Amiga
// emulator and run the demo! DeliPlayer does a good job of playing them
// (there are some occasional error mostly concerning vibrato and portamento
// effects, but I can live with that!), and it can be downloaded from
// http://www.deliplayer.com
//
// ---
//
// From http://www.cactus.jawnet.pl/attitude/index.php?action=readtext&issue=12&which=12
//
// [Bepp] For your final Amiga release, the music disk His Master's Noise,
// you developed a special version of NoiseTracker. Could you tell us a
// little about this project?
//
// [Mahoney] I wanted to make a music disk with loads of songs, without being
// too repetitive or boring. So all of my "experimental features" that did not
// belong to NoiseTracker v2.0 were put into a separate version that would
// feature wavetable sounds, chord calculations, off-line filter calculations,
// mixing, reversing, sample accurate delays, resampling, fades - calculations
// that would be done on a standard setup of sounds instead of on individual
// modules. This "compression technique" lead to some 100 songs fitting on two
// standard 3.5" disks, written by 22 different composers. I'd say that writing
// a music program does give you loads of talented friends - you should try
// that yourself someday!
//
// ---
//
// From: Pex Tufvesson
// To: Claudio Matsuoka
// Date: Sat, Jun 1, 2013 at 4:16 AM
// Subject: Re: A question about (very) old stuff
//
// (...)
// If I remember correctly, these chip sounds were done with several short
// waveforms, and an index table that was loopable that would choose which
// waveform to play each frame. And, you didn't have to "draw" every
// waveform in the instrument - you would choose which waveforms to draw
// and the replayer would (at startup) interpolate the waveforms that you
// didn't draw.
//
// In the special noisetracker, you could draw all of these waveforms, draw
// the index table, and the instrument would be stored in one of the
// "patterns" of the song.


haze::Player *HisMastersNoise::new_player(void *buf, uint32_t size, int sr)
{
    return new HMN_Player(buf, size, sr);
}

//----------------------------------------------------------------------

HMN_Player::HMN_Player(void *buf, uint32_t size, int sr) :
    AmigaPlayer(buf, size, sr),
    L695_counter(6),
    L642_speed(6),
    L693_songpos(0),
    L692_pattpos(0),
    L701_dmacon(0),
    L698_samplestarts{0},
    L681_pattbrk(false)
{
    memset(voice, 0, sizeof(voice));
}

HMN_Player::~HMN_Player()
{
}

void HMN_Player::start()
{
    tempo_ = 125.0;
    time_ = 0.0f;

    int num_pat = 0;
    for (int i = 0; i < 128; i++) {
        int pat = mdata.read8(952 + i);
        num_pat = std::max(num_pat, pat);
    }
    num_pat++;

    int offset = 1084 + 1024 * num_pat;
    for (int i = 0; i < 31; i++) {
        L698_samplestarts[i] = offset;
        offset += mdata.read16b(20 + 22 + 30 * i) * 2;
    }

    const int pan = options_.get("pan", 70);
    const int panl = -128 * pan / 100;
    const int panr = 127 * pan / 100;

    paula_->set_pan(0, panl);
    paula_->set_pan(1, panr);
    paula_->set_pan(2, panr);
    paula_->set_pan(3, panl);
}

void HMN_Player::play()
{
    L505_2_music();

    time_ += 20.0;
}

void HMN_Player::reset()
{
}

int HMN_Player::length()
{
    return mdata.read8(950);
}

void HMN_Player::frame_info(haze::FrameInfo& fi)
{
    fi.pos = L693_songpos;
    fi.row = L692_pattpos;
    fi.num_rows = 64;
    fi.frame = L642_speed - L695_counter;
    fi.song = 0;
    fi.speed = L642_speed;
    fi.tempo = 125;

    haze::Player::frame_info(fi);
}

State HMN_Player::save_state()
{
    return to_state<HMN_Player>(*this);
}

void HMN_Player::restore_state(State const& state)
{
    from_state<HMN_Player>(state, *this);
}

//----------------------------------------------------------------------

namespace {

const uint8_t Sin[32] = {
    0x00, 0x18, 0x31, 0x4a, 0x61, 0x78, 0x8d, 0xa1, 0xb4, 0xc5, 0xd4, 0xe0, 0xeb, 0xf4, 0xfa, 0xfd,
    0xff, 0xfd, 0xfa, 0xf4, 0xeb, 0xe0, 0xd4, 0xc5, 0xb4, 0xa1, 0x8d, 0x78, 0x61, 0x4a, 0x31, 0x18
};

const uint8_t MegaArps[256] = {
       0,  3,  7, 12, 15, 12,  7,  3,  0,  3,  7, 12, 15, 12,  7,  3,
       0,  4,  7, 12, 16, 12,  7,  4,  0,  4,  7, 12, 16, 12,  7,  4,
       0,  3,  8, 12, 15, 12,  8,  3,  0,  3,  8, 12, 15, 12,  8,  3,
       0,  4,  8, 12, 16, 12,  8,  4,  0,  4,  8, 12, 16, 12,  8,  4,
       0,  5,  8, 12, 17, 12,  8,  5,  0,  5,  8, 12, 17, 12,  8,  5,
       0,  5,  9, 12, 17, 12,  9,  5,  0,  5,  9, 12, 17, 12,  9,  5,
      12,  0,  7,  0,  3,  0,  7,  0, 12,  0,  7,  0,  3,  0,  7,  0,
      12,  0,  7,  0,  4,  0,  7,  0, 12,  0,  7,  0,  4,  0,  7,  0,

       0,  3,  7,  3,  7, 12,  7, 12, 15, 12,  7, 12,  7,  3,  7,  3,
       0,  4,  7,  4,  7, 12,  7, 12, 16, 12,  7, 12,  7,  4,  7,  4,
      31, 27, 24, 19, 15, 12,  7,  3,  0,  3,  7, 12, 15, 19, 24, 27,
      31, 28, 24, 19, 16, 12,  7,  4,  0,  4,  7, 12, 16, 19, 24, 28,
       0, 12,  0, 12,  0, 12,  0, 12,  0, 12,  0, 12,  0, 12,  0, 12,
       0, 12, 24, 12,  0, 12, 24, 12,  0, 12, 24, 12,  0, 12, 24, 12,
       0,  3,  0,  3,  0,  3,  0,  3,  0,  3,  0,  3,  0,  3,  0,  3,
       0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4
};

#define PERIODS_LEN 38
const int16_t L602_periods[PERIODS_LEN] = {
    0x0358, 0x0328, 0x02fa, 0x02d0, 0x02a6, 0x0280, 0x025c, 0x023a, 0x021a, 0x01fc, 0x01e0,
    0x01c5, 0x01ac, 0x0194, 0x017d, 0x0168, 0x0153, 0x0140, 0x012e, 0x011d, 0x010d, 0x00fe,
    0x00f0, 0x00e2, 0x00d6, 0x00ca, 0x00be, 0x00b4, 0x00aa, 0x00a0, 0x0097, 0x008f, 0x0087,
    0x007f, 0x0078, 0x0071, 0x0000, 0x0000
};

}  // namespace


void HMN_Player::L505_2_music()
{
    L695_counter++;
    if (L642_speed > L695_counter) {
        // L505_4_nonew
        // INGETAVSTANG
        for (int chn = 0; chn < 4; chn++) {
            L577_2_checkcom(chn);
            ProgHandler(chn);
            auto& ch = voice[chn];
            paula_->set_start(chn, ch.n_a_loopstart);  // POI
            paula_->set_length(chn, ch.n_e_replen);    // REPLEN
        }
        return;
    }

    L695_counter = 0;                    // CLR.L   L695
    L505_F_getnew();                     // BRA     L505
}

void HMN_Player::L505_8_arpeggio(const int chn)
{
    auto& ch = voice[chn];

    int val;
    switch (L695_counter % 3) {
    case 2:
        val = ch.n_3_cmdlo & 15;  // L505_A
        break;
    case 0:
        val = 0;                  // L505_B
        break;
    default:
        val = ch.n_3_cmdlo >> 4;
    }

    // L505_C
    for (int i = 0; i < 36; i++) {
        if ((ch.n_10_period & 0xfff) >= L602_periods[i]) {
            if (i + val < PERIODS_LEN) {  // add sanity check
                // L505_E
                percalc(chn, L602_periods[i + val]);
                return;
            }
        }
    }
}

void HMN_Player::L505_F_getnew()
{
    const int pat = mdata.read8(952 + L693_songpos);
    L701_dmacon = 0;

    L505_J_playvoice(pat, 0);
    L505_J_playvoice(pat, 1);
    L505_J_playvoice(pat, 2);
    L505_J_playvoice(pat, 3);

    // L505_M_setdma
    ProgHandler(3);  // ?
    ProgHandler(2);
    ProgHandler(1);
    ProgHandler(0);

    paula_->start_dma(L701_dmacon);
    for (int chn = 3; chn >= 0; chn--) {
        auto& ch = voice[chn];
        paula_->set_start(chn, ch.n_a_loopstart);
        paula_->set_length(chn, ch.n_e_replen);
    }

    // L505_R
    // L505_HA
    L692_pattpos++;
    while (true) {
        if (L692_pattpos == 64) {
            // HAHA
            L692_pattpos = 0;                               // CLR.L   L692
            L681_pattbrk = false;                           // CLR.W   L681
            L693_songpos++;                                 // ADDQ.L  #1,L693
            L693_songpos &= 0x7f;                           // AND.L   #$7F,L693
            if (L693_songpos >= mdata.read8(950)) {
                if (mdata.read8(951) < mdata.read8(950)) {  // sanity check
                    L693_songpos = mdata.read8(951);        // MOVE.B  $3B7(A0),D0 / MOVE.L  D0,L693
                } else {
                    L693_songpos = 0;
                }
            }
        }
        // L505_U
        if (L681_pattbrk) {                                 // TST.W   L681
            L692_pattpos = 64;
            continue;                                       // BNE.S   HAHA
        }
        break;
    }
}

void HMN_Player::L505_J_playvoice(const int pat, const int chn)
{
    const uint32_t event = mdata.read32b(1084 + pat * 1024 + L692_pattpos * 16 + chn * 4);

    auto& ch = voice[chn];

    ch.n_0_note = event >> 16;       // MOVE.L  (A0,D1.L),(A6)
    ch.n_2_cmd = (event & 0xff00) >> 8;
    ch.n_3_cmdlo = event & 0xff;

    const int ins = ((ch.n_0_note & 0xf000) >> 8) | ((ch.n_2_cmd & 0xf0) >> 4);

    if (ins > 0 && ins <= 31) {  // sanity check added: was: ins != 0
        const int ofs = 20 + 30 * (ins - 1) + 22;

        ch.n_4_samplestart = L698_samplestarts[ins - 1];               // MOVE.L  $0(A1,D2.L),$04(A6)     ;instrmemstart
        ch.n_1e_finetune = mdata.read8i(ofs - 0x16 + 0x18);            // MOVE.B  -$16+$18(A3,D4.L),$1E(A6)       ;CURRSTAMM
        ch.n_1c_prog_on = false;                                       // clr.b   $1c(a6) ;prog on/off

        if (!memcmp(mdata.ptr(ofs - 0x16), "Mupp", 4)) {               // CMP.L   #'Mupp',-$16(a3,d4.l)
            ch.n_1c_prog_on = true;                                    // move.b  #1,$1c(a6)      ;prog on

            const uint8_t pattno = mdata.read8(ofs - 0x16 + 4);        // move.b  -$16+$4(a3,d4.l),d2     ;pattno
            ch.n_4_samplestart = 1084 + 1024 * pattno;                 // mulu    #$400,d2
                                                                       // lea     (a0,d2.l),a0
                                                                       // move.l  a0,4(a6)        ;proginstr data-start
            ch.prog_ins = ins - 1;
            // sample data is in patterns!
            const int datastart = ch.n_4_samplestart;
            ch.n_12_volume = mdata.read8(datastart + 0x3c0) & 0x7f;    // MOVE.B  $3C0(A0),$12(A6) / AND.B   #$7F,$12(A6)
            const uint8_t wave_num = mdata.read8(datastart + 0x380);   // move.b  $380(a0),d2
            ch.n_a_loopstart = datastart + wave_num * 32;              // mulu    #$20,d2
                                                                       // lea     (a0,d2.w),a0
                                                                       // move.l  a0,$a(a6)       ;loopstartmempoi = startmempoi
            ch.n_13_volume = mdata.read8(ofs + 3);                     // move.B  $3(a3,d4.l),$13(a6)     ;volume
            ch.n_8_dataloopstart = mdata.read8(ofs - 0x16 + 5);        // move.b  -$16+$5(a3,d4.l),8(a6)  ;dataloopstart
            ch.n_9_dataloopend = mdata.read8(ofs - 0x16 + 6);          // move.b  -$16+$6(a3,d4.l),9(a6)  ;dataloopend
            ch.n_8_length = mdata.read16b(ofs - 0x16 + 5);             // ouch! that was a nasty variable reuse trick
            ch.n_e_replen = 0x10;                                      // move.w  #$10,$e(a6)     ;looplen
        } else {
            // noprgo
            ch.n_8_length = mdata.read16b(ofs);                        // MOVE.W  $0(A3,D4.L),$08(A6)
            ch.n_13_volume = mdata.read16b(ofs + 2);                   // MOVE.W  $2(A3,D4.L),$12(A6)
            ch.n_12_volume = 0x40;                                     // move.b  #$40,$12(a6)
            const uint16_t repeat = mdata.read16b(ofs + 4);            // MOVE.W  $4(A3,D4.L),D3
            if (repeat) {                                              // TST.W   D3
                ch.n_a_loopstart = ch.n_4_samplestart + repeat;        // ADD.L   D3,D2 / MOVE.L  D2,$A(A6)       ;LOOPSTARTPOI
                ch.n_8_length = repeat + mdata.read16b(ofs + 6);       // MOVE.W  $4(A3,D4.L),D0  ;REPEAT
                                                                       // ADD.W   $6(A3,D4.L),D0  ;+REPLEN
                                                                       // MOVE.W  D0,$8(A6)       ;STARTLENGTH
                ch.n_e_replen = mdata.read16b(ofs + 6);                // MOVE.W  $6(A3,D4.L),$E(A6);LOOPLENGTH
            } else {
                // L505_K_noloop
                ch.n_e_replen = mdata.read16b(ofs + 6);
            }
        }
    }

    // L505_L_setregs
    if (ch.n_0_note & 0xfff) {
        if (ch.n_8_length) {              // TST.W  8(A6) / BEQ.S  STOPSOUNDET
            switch (ch.n_2_cmd & 0xf) {
            case 0x05:  // MYPI
                setmyport(chn);
                L577_2_checkcom2(chn);
                break;
            case 0x03:
                setmyport(chn);
                L577_2_checkcom2(chn);
                break;
            default:    // JUP
                ch.n_10_period = ch.n_0_note & 0xfff;
                paula_->stop_dma(1 << chn);
                ch.n_1b_vibpos = 0;                    // CLR.B   $1B(A6)
                ch.n_1d_prog_datacou = 0;              // clr.b   $1d(a6) ;proglj-datacou
                if (ch.n_1c_prog_on) {
                    paula_->set_start(chn, ch.n_a_loopstart);
                    paula_->set_length(chn, ch.n_e_replen);
                } else {
                    // normalljudstart
                    paula_->set_start(chn, ch.n_4_samplestart);
                    paula_->set_length(chn, ch.n_8_length);
                }
                // onormalljudstart
                percalc(chn, ch.n_10_period & 0xfff);
                L701_dmacon |= 1 << chn;
            }
        } else {
            // STOPSOUNDET
            paula_->stop_dma(1 << chn);
        }
    }

    // EFTERSTOPSUND
    // L505_L_setregs2
    L577_2_checkcom2(chn);
}

void HMN_Player::ProgHandler(const int chn)
{
    auto& ch = voice[chn];

    if (ch.n_1c_prog_on) {
        uint8_t datacou = ch.n_1d_prog_datacou;                    // move.b  $1d(a6),d0      ;datacou
        const int datastart = ch.n_4_samplestart;                  // move.l  $4(a6),a0       ;datastart

        const int index = 0x380 + datacou;
        ch.n_12_volume = mdata.read8(datastart + 0x40 + index) & 0x7f;  // progvolume
        ch.n_a_loopstart = datastart + mdata.read8(datastart + index) * 32;     // loopstartmempoi
        datacou++;
        if (datacou > ch.n_9_dataloopend) {
            datacou = ch.n_8_dataloopstart;
        }
        // norestartofdata
        ch.n_1d_prog_datacou = datacou;
    }
    // norvolum
    const uint8_t volume = (ch.n_12_volume * ch.n_13_volume) >> 6;
    paula_->set_volume(chn, volume);
}

void HMN_Player::setmyport(const int chn)
{
    auto& ch = voice[chn];
    ch.n_18_wantperiod = ch.n_0_note & 0xfff;
    ch.n_16_portdir = false;            // clr.b   $16(a6)
    if (ch.n_10_period == ch.n_18_wantperiod) {
        // clrport
        ch.n_18_wantperiod = 0;         // clr.w   $18(a6)
    } else if (ch.n_10_period < ch.n_18_wantperiod) {
        ch.n_16_portdir = true;         // move.b  #$1,$16(a6)
    }
}

void HMN_Player::myport(const int chn)
{
    auto& ch = voice[chn];
    if (ch.n_3_cmdlo) {
        ch.n_17_toneportspd = ch.n_3_cmdlo;
        ch.n_3_cmdlo = 0;
    }
    myslide(chn);
}

void HMN_Player::myslide(const int chn)
{
    auto& ch = voice[chn];

    if (ch.n_18_wantperiod) {
        if (ch.n_16_portdir) {
            ch.n_10_period += ch.n_17_toneportspd;
            if (ch.n_10_period > ch.n_18_wantperiod) {
                ch.n_10_period = ch.n_18_wantperiod;
                ch.n_18_wantperiod = 0;
            }
        } else {
            // MYSUB
            ch.n_10_period -= ch.n_17_toneportspd;
            if (ch.n_10_period < ch.n_18_wantperiod) {
                ch.n_10_period = ch.n_18_wantperiod;
                ch.n_18_wantperiod = 0;
            }
        }
    }
    paula_->set_period(chn, ch.n_10_period);  // move.w  $10(a6),$6(a5)
}

void HMN_Player::vib(const int chn)
{
    auto& ch = voice[chn];
    if (ch.n_3_cmdlo) {
        ch.n_1a_vibrato = ch.n_3_cmdlo;
    }
    vibrato(chn);
}

void HMN_Player::vibrato(const int chn)
{
    auto& ch = voice[chn];

    const uint8_t pos = (ch.n_1b_vibpos >> 2) & 0x1f;
    const uint8_t val = Sin[pos];
    const int16_t amt = (val * (ch.n_1a_vibrato & 0xf)) >> 7;

    int16_t period = ch.n_10_period;
    if ((ch.n_1b_vibpos & 0x80) == 0) {
        period += amt;
    } else {
        // VIBMIN
        period -= amt;
    }

    percalc(chn, period);
    ch.n_1b_vibpos += (ch.n_1a_vibrato >> 2) & 0x3c;
}

void HMN_Player::megaarp(const int chn)
{
    auto& ch = voice[chn];
    const uint8_t pos = ch.n_1b_vibpos;
    ch.n_1b_vibpos++;
    const int index = ((ch.n_3_cmdlo & 0xf) << 4) + (pos & 0xf);
    const uint8_t val = MegaArps[index];

    // MegaAlo
    for (int i = 0; i < 36; i++) {
        if ((ch.n_10_period & 0xfff) >= L602_periods[i]) {
            int idx = i + val;
            while (idx >= PERIODS_LEN) {
                idx -= 12;
            }
            // MegaOk
            percalc(chn, L602_periods[index]);
            return;
        }
    }
}

void HMN_Player::percalc(const int chn, int16_t val)
{
    paula_->set_period(chn, ((voice[chn].n_1e_finetune * val) >> 8) + val);
}


void HMN_Player::L577_2_checkcom(const int chn)
{
    auto& ch = voice[chn];
    const uint8_t cmd = ch.n_2_cmd & 0xf;
    if (cmd == 0 && ch.n_3_cmdlo == 0) {
        // NEJDU
        percalc(chn, ch.n_10_period);
        return;
    }
    switch (cmd) {
    case 0x0:
        L505_8_arpeggio(chn);
        break;
    case 0x1:
        L577_7_portup(chn);
        break;
    case 0x2:
        L577_9_portdown(chn);
        break;
    case 0x3:
        myport(chn);
        break;
    case 0x4:
        vib(chn);
        break;
    case 0x5:
        myportvolslide(chn);
        break;
    case 0x6:
        vibvolslide(chn);
        break;
    case 0x7:
        megaarp(chn);
        break;
    default:
        percalc(chn, ch.n_10_period);
        if (cmd == 0xa) {
            L577_3_volslide(chn);
        }
    }
}

void HMN_Player::L577_3_volslide(const int chn)
{
    auto& ch = voice[chn];
    if ((ch.n_3_cmdlo >> 4) == 0) {
        const uint8_t cmdlo = ch.n_3_cmdlo & 0x0f;
        if (ch.n_13_volume > cmdlo) {
            ch.n_13_volume -= cmdlo;
        } else {
            ch.n_13_volume = 0;
        }
    } else {
        ch.n_13_volume += ch.n_3_cmdlo >> 4;
        if (ch.n_13_volume > 0x40) {
            ch.n_13_volume = 0x40;
        }
    }
}

void HMN_Player::vibvolslide(const int chn)
{
    vibrato(chn);
    L577_3_volslide(chn);
}

void HMN_Player::myportvolslide(const int chn)
{
    myslide(chn);
    L577_3_volslide(chn);
}

void HMN_Player::L577_7_portup(const int chn)
{
    auto& ch = voice[chn];
    ch.n_10_period -= ch.n_3_cmdlo;
    if ((ch.n_10_period & 0xfff) < 0x71) {
        ch.n_10_period = ch.n_10_period & 0xf000;
        ch.n_10_period |= 0x71;
    }
    // L577_8
    percalc(chn, ch.n_10_period);
}

void HMN_Player::L577_9_portdown(const int chn)
{
    auto& ch = voice[chn];
    ch.n_10_period += ch.n_3_cmdlo;
    if ((ch.n_10_period & 0xfff) >= 0x358) {
        ch.n_10_period = ch.n_10_period & 0xf000;
        ch.n_10_period |= 0x358;
    }
    // L577_A
    percalc(chn, ch.n_10_period);
}

void HMN_Player::L577_2_checkcom2(const int chn)
{
    switch (voice[chn].n_2_cmd & 0xf) {
    case 0xe:
        L577_H_setfilt(chn);
        break;
    case 0xd:
        L577_i_pattbreak();
        break;
    case 0xb:
        L577_J_mt_posjmp(chn);
        break;
    case 0xc:
        L577_K_setvol(chn);
        break;
    case 0xf:
        L577_M_setspeed(chn);
        break;
    default:
        L577_2_checkcom(chn);
    }
}

void HMN_Player::L577_H_setfilt(const int chn)
{
    auto& ch = voice[chn];
    paula_->enable_filter((ch.n_3_cmdlo & 0x0f) != 0);
}

void HMN_Player::L577_i_pattbreak()
{
    L681_pattbrk = !L681_pattbrk;
}

void HMN_Player::L577_J_mt_posjmp(const int chn)
{
    auto& ch = voice[chn];
    L693_songpos = ch.n_3_cmdlo - 1;
    L681_pattbrk = !L681_pattbrk;
}

void HMN_Player::L577_K_setvol(const int chn)
{
    auto& ch = voice[chn];
    if (ch.n_3_cmdlo > 0x40) {          // cmp.b   #$40,$3(a6)
        ch.n_3_cmdlo = 40;              // move.b  #$40,$3(a6)
    }
    ch.n_13_volume = ch.n_3_cmdlo;
}

void HMN_Player::L577_M_setspeed(const int chn)
{
    auto& ch = voice[chn];
    if (ch.n_3_cmdlo > 0x1f) {          // cmp.b   #$1f,$3(a6)
        ch.n_3_cmdlo = 0x1f;            // move.b  #$1f,$3(a6)
    }
    // mt_sets
    if (ch.n_3_cmdlo != 0) {
        L642_speed = ch.n_3_cmdlo;      // move.b  d0,L642_speed
        L695_counter = 0;               // clr.b   L695_counter
    }
}

// Everything is under control,
// but what is that good for?

