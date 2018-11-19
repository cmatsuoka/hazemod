#include "player/nt11.h"

// NT1.1 Replayer
//
// A player based on the Noisetracker V1.1 play routine by Pex Tufvesson
// and Anders Berkeman (Mahoney & Kaktus - HALLONSOFT 1989).


haze::Player *Noisetracker::new_player(void *buf, uint32_t size, int sr)
{
    return new NT11_Player(buf, size, sr);
}

//----------------------------------------------------------------------

NT11_Player::NT11_Player(void *buf, uint32_t size, int sr) :
    Player(buf, size, 4, sr),
    mt_speed(6),
    mt_songpos(0),
    mt_pattpos(0),
    mt_counter(mt_speed),
    mt_break(false),
    mt_samplestarts{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
{
    memset(mt_voice, 0, sizeof(mt_voice));
}

NT11_Player::~NT11_Player()
{
}

void NT11_Player::start()
{
    speed_ = 6;
    tempo_ = 125.0;
    time_ = 0.0f;

    initial_speed_ = speed_;
    initial_tempo_ = tempo_;

    int num_pat = 0;
    for (int i = 0; i < 128; i++) {
        int pat = mdata.read8(952 + i);
        num_pat = std::max(num_pat, pat);
    }
    num_pat++;

    int offset = 1084 + 1024 * num_pat;
    for (int i = 0; i < 31; i++) {
        mt_samplestarts[i] = offset;
        offset += mdata.read16b(20 + 22 + 30 * i) * 2;
    }

    mixer_->add_sample(mdata.ptr(0), mdata.size());
    mixer_->set_sample(0, 0);
    mixer_->set_sample(1, 0);
    mixer_->set_sample(2, 0);
    mixer_->set_sample(3, 0);

    const int pan = options.get("pan", 70);
    const int panl = -128 * pan / 100;
    const int panr = 127 * pan / 100;

    mixer_->set_pan(0, panl);
    mixer_->set_pan(1, panr);
    mixer_->set_pan(2, panr);
    mixer_->set_pan(3, panl);
}

void NT11_Player::play()
{
    mt_music();

    time_ += 20.0;

}

void NT11_Player::reset()
{
}

void NT11_Player::frame_info(haze::FrameInfo& pi)
{
    pi.pos = mt_songpos;
    pi.row = mt_pattpos;
    pi.num_rows = 64;
    pi.frame = mt_speed - mt_counter;
    pi.song = 0;
    pi.speed = mt_speed;
    pi.tempo = 125;
    pi.time = time_;
}

//----------------------------------------------------------------------

namespace {

const uint8_t mt_sin[32] = {
    0x00, 0x18, 0x31, 0x4a, 0x61, 0x78, 0x8d, 0xa1, 0xb4, 0xc5, 0xd4, 0xe0, 0xeb, 0xf4, 0xfa, 0xfd,
    0xff, 0xfd, 0xfa, 0xf4, 0xeb, 0xe0, 0xd4, 0xc5, 0xb4, 0xa1, 0x8d, 0x78, 0x61, 0x4a, 0x31, 0x18
};

const int16_t mt_periods[38] = {
    0x0358, 0x0328, 0x02fa, 0x02d0, 0x02a6, 0x0280, 0x025c, 0x023a, 0x021a, 0x01fc, 0x01e0,
    0x01c5, 0x01ac, 0x0194, 0x017d, 0x0168, 0x0153, 0x0140, 0x012e, 0x011d, 0x010d, 0x00fe,
    0x00f0, 0x00e2, 0x00d6, 0x00ca, 0x00be, 0x00b4, 0x00aa, 0x00a0, 0x0097, 0x008f, 0x0087,
    0x007f, 0x0078, 0x0071, 0x0000, 0x0000
};

}  // namespace


void NT11_Player::mt_music()
{
    mt_counter++;

    if (mt_speed > mt_counter) {
        // mt_nonew
        mt_checkcom(0);
        mt_checkcom(1);
        mt_checkcom(2);
        mt_checkcom(3);

        // mt_endr
        if (mt_break) {
            mt_nex();
        }
        return;
    }

    mt_counter = 0;
    mt_getnew();
}

void NT11_Player::mt_arpeggio(const int chn)
{
    auto& ch = mt_voice[chn];

    int val;
    switch (mt_counter % 3) {
    case 2:  // mt_arp1
       val = ch.n_3_cmdlo & 15;
       break;
    case 0:  // mt_arp2
       val = 0;
       break;
    default:
       val = ch.n_3_cmdlo >> 4;
    }

    // mt_arp3
    for (int i = 0; i < 36; i++) {
        if (ch.n_10_period >= mt_periods[i]) {
            if (i + val < 38) {  // sanity check
                // mt_arp4
                mixer_->set_period(chn, mt_periods[i + val]);  // move.w  d2,$6(a5)
                return;
            }
        }
    }
}

void NT11_Player::mt_getnew()
{
    const int pat = mdata.read8(952 + mt_songpos);

    mt_playvoice(pat, 0);
    mt_playvoice(pat, 1);
    mt_playvoice(pat, 2);
    mt_playvoice(pat, 3);

    // mt_setdma
    for (int chn = 0; chn < 4; chn++) {
        auto& ch = mt_voice[chn];
        mixer_->set_loop_start(chn, ch.n_a_loopstart);
        mixer_->set_loop_end(chn, ch.n_a_loopstart + ch.n_e_replen * 2);
    }

    mt_pattpos++;
    if (mt_pattpos == 64) {
        mt_nex();
    }
}

void NT11_Player::mt_playvoice(const int pat, const int chn)
{
    const uint32_t event = mdata.read32b(1084 + pat * 1024 + mt_pattpos * 16 + chn * 4);

    auto& ch = mt_voice[chn];

    ch.n_0_note = event >> 16;   // move.l  (a0,d1.l),(a6)
    ch.n_2_cmd = (event & 0xff00) >> 8;
    ch.n_3_cmdlo = event & 0xff;

    const int ins = ((ch.n_0_note & 0xf000) >> 8) | ((ch.n_2_cmd & 0xf0) >> 4);

    if (ins > 0 && ins <= 31) {  // sanity check added: was: ins != 0
        const int ofs = 20 + 30 * (ins - 1) + 22;
        ch.n_4_samplestart = mt_samplestarts[ins - 1];
        ch.n_8_length = mdata.read16b(ofs);                   // move.w  (a3,d4.l),$8(a6)
        ch.n_12_volume = mdata.read8(ofs + 3);                // move.w  $2(a3,d4.l),$12(a6)

        const uint32_t repeat = mdata.read16b(ofs + 4);

        if (repeat) {
            ch.n_a_loopstart = ch.n_4_samplestart + repeat * 2;
            ch.n_8_length = repeat + mdata.read16b(ofs + 6);
            ch.n_e_replen = mdata.read16b(ofs + 6);           // move.w  $6(a3,d4.l),$e(a6)
            mixer_->set_volume(chn, ch.n_12_volume << 2);     // move.w  $12(a6),$8(a5)
        } else {
            // mt_noloop
            ch.n_a_loopstart = repeat;
            ch.n_e_replen = mdata.read16b(ofs + 6);
            mixer_->set_volume(chn, ch.n_12_volume << 2);     // move.w  $12(a6),$8(a5)
        }
        mixer_->enable_loop(chn, repeat != 0);
    }

    // mt_setregs
    if (mt_voice[chn].n_0_note & 0xff) {
        if ((mt_voice[chn].n_2_cmd & 0xf) == 0x3) {
            mt_setmyport(chn);
            mt_checkcom2(chn);
        } else {
            mt_setperiod(chn);
        }
    } else {
        mt_checkcom2(chn);  // If no note
    }
}

void NT11_Player::mt_setperiod(const int chn)
{
    auto& ch = mt_voice[chn];
    ch.n_10_period = ch.n_0_note & 0xfff;
    ch.n_1b_vibpos = 0;                 // clr.b   $1b(a6)
    mixer_->set_start(chn, ch.n_4_samplestart);
    mixer_->set_end(chn, ch.n_4_samplestart + ch.n_8_length * 2);
    mixer_->set_period(chn, ch.n_10_period);
    mt_checkcom2(chn);
}

void NT11_Player::mt_nex()
{
    mt_pattpos = 0;
    mt_break = false;
    mt_songpos++;
    mt_songpos &= 0x7f;

    if (mt_songpos >= mdata.read8(950)) {     // cmp.b   mt_data+$3b6,d1
        // mt_songpos = 0 in Noisetracker 1.0
        // add sanity check: check if value is a valid restart position
        if (mdata.read8(951) < mdata.read8(950)) {
            mt_songpos = mdata.read8(951);    // move.b  mt_data+$3b7,mt_songpos
        } else {
            mt_songpos = 0;
        }
    }
}

void NT11_Player::mt_setmyport(const int chn)
{
    auto& ch = mt_voice[chn];

    ch.n_18_wantperiod = ch.n_0_note & 0xfff;
    ch.n_16_portdir = false;            // clr.b   $16(a6)

    if (ch.n_10_period == ch.n_18_wantperiod) {
        // mt_clrport
        ch.n_18_wantperiod = 0;         // clr.w   $18(a6)
    } else if (ch.n_10_period < ch.n_18_wantperiod) {
        ch.n_16_portdir = true;         // move.b  #$1,$16(a6)
    }
}

void NT11_Player::mt_myport(const int chn)
{
    auto& ch = mt_voice[chn];

    if (ch.n_3_cmdlo) {
         ch.n_17_toneportspd = ch.n_3_cmdlo;
         ch.n_3_cmdlo = 0;
    }

    // mt_myslide
    if (ch.n_18_wantperiod) {
        if (ch.n_16_portdir) {
            ch.n_10_period += ch.n_17_toneportspd;
            if (ch.n_10_period > ch.n_18_wantperiod) {
                ch.n_10_period = ch.n_18_wantperiod;
                ch.n_18_wantperiod = 0;
            }
        } else {
            // mt_mysub
            ch.n_10_period -= ch.n_17_toneportspd;
            if (ch.n_10_period < ch.n_18_wantperiod) {
                ch.n_10_period = ch.n_18_wantperiod;
                ch.n_18_wantperiod = 0;
            }
        }
    }

    mixer_->set_period(chn, ch.n_10_period);  // move.w  $10(a6),$6(a5)
}

void NT11_Player::mt_vib(const int chn)
{
    auto& ch = mt_voice[chn];

    if (ch.n_3_cmdlo) {
        ch.n_1a_vibrato = ch.n_3_cmdlo;
    }

    // mt_vi
    const int pos = (ch.n_1b_vibpos >> 2) & 0x1f;
    const int amt = (mt_sin[pos] * (ch.n_1a_vibrato & 0xf)) >> 6;

    int period = ch.n_10_period;
    if ((ch.n_1b_vibpos & 0x80) == 0) {
        period += amt;
    } else {
        // mt_vibmin
        period -= amt;
    }

    mixer_->set_period(chn, period);
    ch.n_1b_vibpos += (ch.n_1a_vibrato >> 2) & 0x3c;
}

void NT11_Player::mt_checkcom(const int chn)
{
    auto& ch = mt_voice[chn];

    const int cmd = ch.n_2_cmd & 0xf;

    switch (cmd) {
    case 0x0:
        mt_arpeggio(chn);
        break;
    case 0x1:
        mt_portup(chn);
        break;
    case 0x2:
        mt_portdown(chn);
        break;
    case 0x3:
        mt_myport(chn);
        break;
    case 0x4:
        mt_vib(chn);
        break;
    default:
        mixer_->set_period(chn, ch.n_10_period);  // move.w  $10(a6),$6(a5)
        if (cmd == 0x0a) {
            mt_volslide(chn);
        }
    }
}

void NT11_Player::mt_volslide(const int chn)
{
    auto& ch = mt_voice[chn];

    if ((ch.n_3_cmdlo >> 4) == 0) {
        // mt_voldown
        uint8_t cmdlo = ch.n_3_cmdlo & 0x0f;
        if (ch.n_12_volume > cmdlo) {
            ch.n_12_volume -= cmdlo;
        } else {
            ch.n_12_volume = 0;
        }
    } else {
        ch.n_12_volume += ch.n_3_cmdlo >> 4;
        if (ch.n_12_volume > 0x40) {
            ch.n_12_volume = 0x40;
        }
    }
    // mt_vol2
    mixer_->set_volume(chn, ch.n_12_volume << 2);  // move.w  $12(a6),$8(a5)
}

void NT11_Player::mt_portup(const int chn)
{
    auto& ch = mt_voice[chn];

    ch.n_10_period -= ch.n_3_cmdlo;

    if ((ch.n_10_period & 0xfff) < 0x71) {
        ch.n_10_period = ch.n_10_period & 0xf000;
        ch.n_10_period |= 0x71;
    }
    // mt_por2
    mixer_->set_period(chn, ch.n_10_period & 0xfff);  // move.w $10(a6),d0; and.w #$fff,d0; move.w d0,$6(a5)
}

void NT11_Player::mt_portdown(const int chn)
{
    auto& ch = mt_voice[chn];

    ch.n_10_period += ch.n_3_cmdlo;

    if ((ch.n_10_period & 0xfff) >= 0x358) {
        ch.n_10_period = ch.n_10_period & 0xf000;
        ch.n_10_period |= 0x358;
    }
    mixer_->set_period(chn, ch.n_10_period & 0xfff);  // move.w $10(a6),d0; and.w #$fff,d0; move.w d0,$6(a5)
}

void NT11_Player::mt_checkcom2(const int chn)
{
    switch (mt_voice[chn].n_2_cmd & 0xf) {
    case 0xe:
        mt_setfilt(chn);
        break;
    case 0xd:
        mt_pattbreak();
        break;
    case 0xb:
        mt_posjmp(chn);
        break;
    case 0xc:
        mt_setvol(chn);
        break;
    case 0xf:
        mt_setspeed(chn);
        break;
    }
}

void NT11_Player::mt_setfilt(const int chn)
{
    auto& ch = mt_voice[chn];
    mixer_->enable_filter((ch.n_3_cmdlo & 0x0f) != 0);
}


void NT11_Player::mt_pattbreak()
{
    mt_break = !mt_break;
}

void NT11_Player::mt_posjmp(const int chn)
{
    auto& ch = mt_voice[chn];
    mt_songpos -= ch.n_3_cmdlo;
    mt_break = !mt_break;
}

void NT11_Player::mt_setvol(const int chn)
{
    auto& ch = mt_voice[chn];

    if (ch.n_3_cmdlo > 0x40) {     // cmp.b   #$40,$3(a6)
        ch.n_3_cmdlo = 40;         // move.b  #$40,$3(a6)
    }
    // mt_vol4
    mixer_->set_volume(chn, ch.n_3_cmdlo << 2);  // move.b  $3(a6),$8(a5)
    // oxdz fix: otherwise we're overriden by set_volume in mt_playvoice()
    ch.n_12_volume = ch.n_3_cmdlo;
}

void NT11_Player::mt_setspeed(const int chn)
{
    auto& ch = mt_voice[chn];
    if (ch.n_3_cmdlo > 0x1f) {     // cmp.b   #$1f,$3(a6)
        ch.n_3_cmdlo = 0x1f;       // move.b  #$1f,$3(a6)
    }
    // mt_sets
    if (ch.n_3_cmdlo != 0) {
        mt_speed = ch.n_3_cmdlo;   // move.b  d0,mt_speed
        mt_counter = 0;            // clr.b   mt_counter
    }
}
