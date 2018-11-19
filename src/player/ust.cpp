#include "player/ust.h"

// Ultimate Soundtracker V27 replayer
//
// A player based on the Ultimate Soundtracker V27, written 1987/1988 by
// Karsten Obarski. "All bugs removed".
//
// > "Just look at it -- so small, innocent and cute. :)"
// > -- Olav "8bitbubsy" Sørensen


haze::Player *UltimateSoundtracker::new_player(void *buf, uint32_t size, int sr)
{
    return new UST_Player(buf, size, sr);
}

//----------------------------------------------------------------------

UST_Player::UST_Player(void *buf, uint32_t size, int sr) :
    Player(buf, size, 4, sr),
    pointers{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    trkpos(0),
    patpos(0),
    numpat(0),
    timpos(0)
{
    memset(datachn, 0, sizeof(datachn));
}

UST_Player::~UST_Player()
{
}

void UST_Player::start()
{
    speed_ = 6;
    tempo_ = 125.0;
    time_ = 0.0f;

    initial_speed_ = speed_;
    initial_tempo_ = tempo_;

    numpat = mdata.read8(470);

    int max_pat = 0;
    for (int i = 0; i < 128; i++) {
        int pat = mdata.read8(472 + i);
        max_pat = std::max(max_pat, pat);
    }

    int offset = 600 + 1024 * (max_pat + 1);
    for (int i = 0; i < 15; i++) {
        pointers[i] = offset;
        offset += mdata.read16b(20 + 22 + 30 * i) * 2;
    }

    mixer_->add_sample(mdata.ptr(0), mdata.size());
    mixer_->set_sample(0, 0);
    mixer_->set_sample(1, 0);
    mixer_->set_sample(2, 0);
    mixer_->set_sample(3, 0);

    int pan = options.get("pan", 70);
    int panl = -128 * pan / 100;
    int panr = 127 * pan / 100;

    mixer_->set_pan(0, panl);
    mixer_->set_pan(1, panr);
    mixer_->set_pan(2, panr);
    mixer_->set_pan(3, panl);
}

void UST_Player::play()
{
    replay_muzak();

    time_ += 20.0;
}

void UST_Player::reset()
{
}

void UST_Player::frame_info(haze::FrameInfo& pi)
{
    pi.pos = trkpos;
    pi.row = patpos;
    pi.num_rows = 64;
    pi.frame = timpos;
    pi.song = 0;
    pi.speed = 6;
    pi.tempo = 125.0;
    pi.time = time_;
}

//----------------------------------------------------------------------

namespace {

const int16_t notetable[37] {
    856, 808, 762, 720, 678, 640, 604, 570,
    538, 508, 480, 453, 428, 404, 381, 360,
    339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143,
    135, 127, 120, 113, 0
};

}  // namespace


//------------------------------------------------
// replay-routine
//------------------------------------------------

void UST_Player::replay_muzak()
{
    if (++timpos == 6) {
        replaystep();
    } else {
        //------------------------------------------------
        // time left to handle effects between steps
        //------------------------------------------------

        // chaneleffects
        if (datachn[0].n_3_effect_number) { ceff5(0); }
        if (datachn[1].n_3_effect_number) { ceff5(1); }
        if (datachn[2].n_3_effect_number) { ceff5(2); }
        if (datachn[3].n_3_effect_number) { ceff5(3); }
    }
}

void UST_Player::ceff5(const int chn)
{
    switch (datachn[chn].n_2_sound_number & 0x0f) {
    case 1:
        arpreggiato(chn);
        break;
    case 2:
        pitchbend(chn);
        break;
    }
}

//------------------------------------------------
// effect 1 arpreggiato
//------------------------------------------------

void UST_Player::arpreggiato(const int chn)  // ** spread by time
{
    auto& ch = datachn[chn];

    int val;
    switch (timpos) {           // ** get higher note-values or play original
    case 1:
        val = ch.n_3_effect_number >> 4;    // arp1
        break;
    case 2:
        val = ch.n_3_effect_number & 0x0f;  // arp2
        break;
    case 3:
        val = 0;
        break;
    case 4:
        val = ch.n_3_effect_number >> 4;    // arp1
        break;
    case 5:
        val = ch.n_3_effect_number & 0x0f;  // arp2
        break;
    default:
        val = 0;
    }

    // arp4
    for (int i = 0; i < 36; i++) {
        if (ch.n_16_last_saved_note == notetable[i]) {
            if (i + val < 37) {  // add sanity check
                // mt_endpart
                mixer_->set_period(chn, notetable[i + val]);  // move.w  d2,6(a5)
                return;
            }
        }
    }
}

//------------------------------------------------
// effect 2 pitchbend
//------------------------------------------------

void UST_Player::pitchbend(const int chn)
{
    auto& ch = datachn[chn];
    int16_t val = ch.n_3_effect_number >> 4;
    if (val) {
        ch.n_0_note += val;                  // add.w   d0,(a6)
        mixer_->set_period(chn, ch.n_0_note);  // move.w  (a6),6(a5)
        return;
    }
    // pit2
    val = ch.n_3_effect_number & 0x0f;
    if (val) {
        ch.n_0_note -= val;                  // sub.w   d0,(a6)
        mixer_->set_period(chn, ch.n_0_note);  // move.w  (a6),6(a5)
    }
    // pit3
}

//------------------------------------------------
// handle a further step of 16tel data
//------------------------------------------------

void UST_Player::replaystep()               // ** work next pattern-step
{
    timpos = 0;
    const uint8_t pat = mdata.read8(472 + trkpos);

    chanelhandler(pat, 0);
    chanelhandler(pat, 1);
    chanelhandler(pat, 2);
    chanelhandler(pat, 3);

    // rep5
    if (++patpos == 64) {           // pattern finished ?
        patpos = 0;
        if (++trkpos == numpat) {   // song finished ?
            trkpos = 0;
        }
    }
}

//------------------------------------------------
// proof chanel for actions
//------------------------------------------------

void UST_Player::chanelhandler(const int pat, const int chn)
{
    auto& ch = datachn[chn];
    const uint32_t event = mdata.read32b(600 + 1024 * pat + 16 * patpos + 4 * chn);

    const int16_t note = event >> 16;
    const uint8_t cmd = (event & 0xff00) >> 8;
    const uint8_t cmdlo = event & 0xff;

    ch.n_0_note = note;           // get period & action-word
    ch.n_2_sound_number = cmd;
    ch.n_3_effect_number = cmdlo;

    const int ins = cmd >> 4;     // get nibble for soundnumber
    if (ins) {
        const int ofs = 20 + 30 * (ins - 1) + 22;
        ch.n_4_soundstart = pointers[ins - 1];             // store sample-address
        ch.n_8_soundlength = mdata.read16b(ofs);           // store sample-len in words
        ch.n_18_volume = mdata.read16b(ofs + 2);           // store sample-volume
        mixer_->set_volume(chn, ch.n_18_volume << 2);      // change chanel-volume

        ch.n_10_repeatstart = ch.n_4_soundstart + mdata.read16b(ofs + 4);  // store repeatstart
        ch.n_14_repeatlength = mdata.read16b(ofs + 6);     // store repeatlength
        if (ch.n_14_repeatlength != 1) {
            ch.n_4_soundstart = ch.n_10_repeatstart;       // repstart  = sndstart
            ch.n_8_soundlength = mdata.read16b(ofs + 6);   // replength = sndlength
        }
        mixer_->enable_loop(chn, ch.n_14_repeatlength != 1);
    }

    // chan2
    if (ch.n_0_note) {
        ch.n_16_last_saved_note = ch.n_0_note;             // save note for effect
        mixer_->set_start(chn, ch.n_4_soundstart);
        mixer_->set_end(chn, ch.n_4_soundstart + ch.n_8_soundlength * 2);
        mixer_->set_loop_start(chn, ch.n_10_repeatstart);
        mixer_->set_loop_end(chn, ch.n_10_repeatstart + ch.n_14_repeatlength * 2);
        mixer_->set_period(chn, ch.n_0_note);
        ch.n_20_volume_trigger = ch.n_18_volume;
    }
    // chan4 
}
