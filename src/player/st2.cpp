#include "player/st2.h"

// D.O.C SoundTracker V2.0 replayer
//
// A player based on the D.O.C SoundTracker V2.0 playroutine - Improved and
// "omptimized" by Unknown of D.O.C, Based on the playroutine from TJC.


haze::Player *DOC_Soundtracker_2::new_player(void *buf, uint32_t size, int sr)
{
    return new ST_Player(buf, size, sr);
}

//----------------------------------------------------------------------

ST_Player::ST_Player(void *buf, uint32_t size, int sr) :
    AmigaPlayer(buf, size, sr),
    mt_speed(6),
    mt_partnote(0),
    mt_partnrplay(0),
    mt_counter(0),
    mt_maxpart(0),
    mt_dmacon(0),
    mt_status(false),
    mt_sample1{0}
{
    memset(mt_audtemp, 0, sizeof(mt_audtemp));
}

ST_Player::~ST_Player()
{
}

void ST_Player::start()
{
    speed_ = 6;
    tempo_ = 125.0;
    time_ = 0.0f;

    initial_speed_ = speed_;
    initial_tempo_ = tempo_;

    mt_maxpart = mdata.read8(470);

    int num_pat = 0;
    for (int i = 0; i < 128; i++) {
        const int pat = mdata.read8(472 + i);
        num_pat = std::max(num_pat, pat);
    }
    num_pat++;

    int offset = 600 + 1024 * num_pat;
    for (int i = 0; i < 15; i++) {
        mt_sample1[i] = offset;
        offset += mdata.read16b(20 + 22 + 30 * i) * 2;
    }

    const int pan = options.get("pan", 70);
    const int panl = -128 * pan / 100;
    const int panr = 127 * pan / 100;

    paula_->set_pan(0, panl);
    paula_->set_pan(1, panr);
    paula_->set_pan(2, panr);
    paula_->set_pan(3, panl);
}

void ST_Player::play()
{
    mt_music();

    time_ += 20.0;
}

void ST_Player::reset()
{
}

void ST_Player::frame_info(haze::FrameInfo& pi)
{
    pi.pos = mt_partnrplay;
    pi.row = mt_partnote;
    pi.num_rows = 64;
    pi.frame = mt_counter;
    pi.song = 0;
    pi.speed = mt_speed;
    pi.tempo = 125.0;
    pi.time = time_;
}

//----------------------------------------------------------------------

namespace {

int16_t mt_arpeggio[39] = {
    0x0358, 0x0328, 0x02fa, 0x02d0, 0x02a6, 0x0280, 0x025c,
    0x023a, 0x021a, 0x01fc, 0x01e0, 0x01c5, 0x01ac, 0x0194, 0x017d,
    0x0168, 0x0153, 0x0140, 0x012e, 0x011d, 0x010d, 0x00fe, 0x00f0,
    0x00e2, 0x00d6, 0x00ca, 0x00be, 0x00b4, 0x00aa, 0x00a0, 0x0097,
    0x008f, 0x0087, 0x007f, 0x0078, 0x0071, 0x0000, 0x0000, 0x0000
};

}  // namespace


void ST_Player::mt_music()
{
    mt_counter++;
    // mt_cool
    if (mt_counter == mt_speed) {
        mt_counter = 0;
        mt_rout2();
    }

    // mt_notsix
    for (int chn = 0; chn < 4; chn++) {
        // mt_arpout
        switch (mt_audtemp[chn].n_2_cmd & 0xf) {
        case 0x0:
            mt_arpegrt(chn);
            break;
        case 0x1:
            mt_portup(chn);
            break;
        case 0x2:
            mt_portdown(chn);
            break;
        }
    }
}

void ST_Player::mt_portup(const int chn)
{
    auto& ch = mt_audtemp[chn];
    ch.n_22_last_note -= ch.n_3_cmdlo;           // move.b  3(a6),d0 / sub.w   d0,22(a6)
    if (ch.n_22_last_note < 0x71) {              // cmp.w   #$71,22(a6) / bpl.s   mt_ok1
        ch.n_22_last_note = 0x71;                // move.w  #$71,22(a6)
    }
    // mt_ok1
    paula_->set_period(chn, ch.n_22_last_note);  // move.w  22(a6),6(a5)
}

void ST_Player::mt_portdown(const int chn)
{
    auto& ch = mt_audtemp[chn];
    ch.n_22_last_note += ch.n_3_cmdlo;           // move.b  3(a6),d0 / add.w   d0,22(a6)
    if (ch.n_22_last_note >= 0x358) {            // cmp.w   #$358,22(a6) / bmi.s   mt_ok2
        ch.n_22_last_note = 0x358;               // move.w  #$358,22(a6)
    }
    // mt_ok2
    paula_->set_period(chn, ch.n_22_last_note);  // move.w  22(a6),6(a5)
}

void ST_Player::mt_arpegrt(const int chn)
{
    auto& ch = mt_audtemp[chn];
    int val;
    switch (mt_counter) {
    case 1:   // mt_loop2
    case 4: 
        val = ch.n_3_cmdlo >> 4;
        break;
    case 2:   // mt_loop3
    case 5:
        val = ch.n_3_cmdlo & 0x0f;
        break;
    case 3:   // mt_loop4
    default:
        val = 0;
        break;
    }

    // mt_cont
    for (int i = 0; i < 36; i++) {
        if (ch.n_16_period == mt_arpeggio[i]) {
            if (i + val < 39) {  // add sanity check
                // mt_endpart
                paula_->set_period(chn, mt_arpeggio[i + val]);  // move.w  d2,6(a5)
                return;
            }
        }
    }
}

void ST_Player::mt_rout2()
{
    const uint8_t pat = mdata.read8(472 + mt_partnrplay);
    mt_dmacon = 0;

    mt_playit(pat, 0);
    mt_playit(pat, 1);
    mt_playit(pat, 2);
    mt_playit(pat, 3);

    paula_->start_dma(mt_dmacon);
    for (int chn = 3; chn >= 0; chn--) {
        auto& ch = mt_audtemp[chn];
        if (ch.n_14_replen == 1) {
            paula_->set_start(chn, ch.n_10_loopstart);   // move.l  10(a6),$dff0d0
            paula_->set_length(chn, 1);                  // move.w  #1,$dff0d4
        }
    }

    // mt_voice0
    mt_partnote++;
    while (true) {
        if (mt_partnote == 64) {
            // mt_higher
            mt_partnote = 0;
            mt_partnrplay++;
            if (mt_partnrplay >= mt_maxpart) {
                mt_partnrplay = 0;    // clr.l   mt_partnrplay
            }
        }
        // mt_stop
        if (!mt_status) {
            break;
        }
        mt_status = false;
        mt_partnote = 64;
        break;
    }
}

void ST_Player::mt_playit(const int pat, const int chn)
{
    auto& ch = mt_audtemp[chn];
    const uint32_t event = mdata.read32b(600 + 1024 * pat + 16 * mt_partnote + 4 * chn);

    const int16_t note = event >> 16;
    const uint8_t cmd = (event & 0xff00) >> 8;
    const uint8_t cmdlo = event & 0xff;

    ch.n_0_note = note;      // move.l  (a0,d1.l),(a6)
    ch.n_2_cmd = cmd;
    ch.n_3_cmdlo = cmdlo;

    const int ins = (cmd & 0xf0) >> 4;

    if (ins != 0) {
        const int ofs = 20 + 30 * (ins - 1) + 22;
        ch.n_4_samplestart = mt_sample1[ins - 1];             // move.l  (a1,d2),4(a6)
        ch.n_8_length = mdata.read16b(ofs);                   // move.w  (a3,d4),8(a6)
        ch.n_18_volume = mdata.read16b(ofs + 2);              // move.w  2(a3,d4),18(a6)
        const uint16_t repeat = mdata.read16b(ofs + 4);       // move.w  4(a3,d4),d3
        const uint16_t replen = mdata.read16b(ofs + 6);
        if (repeat) {                                         // tst.w   d3 / beq.s   mt_displace
            ch.n_4_samplestart += repeat;                     // move.l  4(a6),d2 / add.l   d3,d2 / move.l  d2,4(a6)
            ch.n_10_loopstart = ch.n_4_samplestart;           // move.l  d2,10(a6)
            ch.n_8_length = replen;                           // move.w  6(a3,d4),8(a6)
            ch.n_14_replen = replen;                          // move.w  6(a3,d4),14(a6)
            paula_->set_volume(chn, ch.n_18_volume);          // move.w  18(a6),8(a5)
        } else {
            // mt_displace
            ch.n_10_loopstart = ch.n_4_samplestart + repeat;  // move.l  4(a6),d2 / add.l   d3,d2 / move.l  d2,10(a6)
            ch.n_14_replen = replen;                          // move.w  6(a3,d4),14(a6)
            paula_->set_volume(chn, ch.n_18_volume);          // move.w  18(a6),8(a5)
        }
    }

    // mt_nosamplechange
    if (ch.n_0_note) {
        ch.n_16_period = ch.n_0_note;                         // move.w  (a6),16(a6)
        paula_->stop_dma(1 << chn);                           // move.w  20(a6),$dff096
        paula_->set_start(chn, ch.n_4_samplestart);           // move.l  4(a6),(a5)
        paula_->set_length(chn, ch.n_8_length);               // move.w  8(a6),4(a5)
        paula_->set_period(chn, ch.n_0_note);                 // move.w  (a6),6(a5)
        mt_dmacon |= 1 << chn;
    }
    // mt_retrout
    if (ch.n_0_note) {
        ch.n_22_last_note = ch.n_0_note; 
    }

    // mt_nonewper
    switch (ch.n_2_cmd & 0x0f) {
    case 0xb:
        mt_posjmp(chn);
        break;
    case 0xc:
        mt_setvol(chn);
        break;
    case 0xd:
        mt_break();
        break;
    case 0xe:
        mt_setfil(chn);
        break;
    case 0xf:
        mt_setspeed(chn);
        break;
    }
}

void ST_Player::mt_posjmp(const int chn)
{
    auto& ch = mt_audtemp[chn];
    mt_status = !mt_status;
    mt_partnrplay = ch.n_3_cmdlo;
    mt_partnrplay--;
}

void ST_Player::mt_setvol(const int chn)
{
    auto& ch = mt_audtemp[chn];
    paula_->set_volume(chn, ch.n_3_cmdlo);  // move.b  3(a6),8(a5)
}

void ST_Player::mt_break()
{
    mt_status = !mt_status;
}

void ST_Player::mt_setfil(const int chn)
{
    auto& ch = mt_audtemp[chn];
    paula_->enable_filter((ch.n_3_cmdlo & 0x0f) != 0);
}

void ST_Player::mt_setspeed(const int chn)
{
    auto& ch = mt_audtemp[chn];
    if (ch.n_3_cmdlo & 0x0f) {
        mt_counter = 0;            // clr.l   mt_counter
        mt_speed = ch.n_3_cmdlo;   // move.b  d0,mt_cool+5
    }
    // mt_back
}
