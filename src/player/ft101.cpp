#include "player/ft101.h"

// FT101 Replayer
//
// Player based on the FastTracker 1.01 replayer written by Fredrik Huss
// (Mr.H / Triton) in 1992-1993. (Function and variable named after the
// corresponding parts in the Protracker 2.1A playroutine.)


constexpr int InitialSpeed = 6;
constexpr double InitialTempo = 125.0;


haze::Player *FastTracker::new_player(void *buf, uint32_t size, int sr)
{
    return new FT_Player(buf, size, sr);
}

//----------------------------------------------------------------------

FT_Player::FT_Player(void *buf, uint32_t size, int sr) :
    PCPlayer(buf, size, 8, sr),
    ft_speed(InitialSpeed),
    ft_counter(ft_speed),
    ft_song_pos(0),
    ft_pbreak_pos(0),
    ft_pos_jump_flag(false),
    ft_pbreak_flag(false),
    ft_patt_del_time(0),
    ft_patt_del_time_2(0),
    ft_pattern_pos(0),
    ft_current_pattern(0),
    cia_tempo(InitialTempo),
    position_jump_cmd(false)
{
    memset(ft_chantemp, 0, sizeof(ft_chantemp));
}

FT_Player::~FT_Player()
{
}

void FT_Player::start()
{
    tempo_ = InitialTempo;
    time_ = 0.0f;

    int num_pat = 0;
    for (int i = 0; i < 128; i++) {
        int pat = mdata.read8(952 + i);
        num_pat = std::max(num_pat, pat);
    }
    num_pat++;

    int offset = 1084 + 1024 * num_pat;
    for (int i = 0; i < 31; i++) {
        //ft_sample_starts[i] = offset;
        offset += mdata.read16b(20 + 22 + 30 * i) * 2;
    }

    const int pan = options_.get("pan", 70);
    const int panl = -128 * pan / 100;
    const int panr = 127 * pan / 100;

    mixer_->set_pan(0, panl);
    mixer_->set_pan(1, panr);
    mixer_->set_pan(2, panr);
    mixer_->set_pan(3, panl);
    mixer_->set_pan(4, panl);
    mixer_->set_pan(5, panr);
    mixer_->set_pan(6, panr);
    mixer_->set_pan(7, panl);
}

void FT_Player::play()
{
    ft_music();

    tempo_ = cia_tempo;
    time_ += 20.0 * 125.0 / tempo_;

    inside_loop_  = ft_chantemp[0].inside_loop;
    inside_loop_ |= ft_chantemp[1].inside_loop;
    inside_loop_ |= ft_chantemp[2].inside_loop;
    inside_loop_ |= ft_chantemp[3].inside_loop;
    inside_loop_ |= ft_chantemp[4].inside_loop;
    inside_loop_ |= ft_chantemp[5].inside_loop;
    inside_loop_ |= ft_chantemp[6].inside_loop;
    inside_loop_ |= ft_chantemp[7].inside_loop;
}

int FT_Player::length()
{
    return mdata.read8(950);
}

void FT_Player::frame_info(haze::FrameInfo& fi)
{
    fi.pos = ft_song_pos;
    fi.row = ft_pattern_pos;
    fi.num_rows = 64;
    fi.frame = (ft_speed - ft_counter + 1) % ft_speed;
    fi.song = 0;
    fi.speed = ft_speed;
    fi.tempo = cia_tempo;

    haze::Player::frame_info(fi);
}

State FT_Player::save_state()
{
    return to_state<FT_Player>(*this);
}

void FT_Player::restore_state(State const& state)
{
    from_state<FT_Player>(state, *this);
}

//----------------------------------------------------------------------

namespace {

extern const uint8_t ArpeggioTable[32];
extern const uint8_t VibratoTable[32];
extern const uint16_t PeriodTable[16 * 36];

uint16_t note_to_period(const uint8_t note, const uint8_t fine)
{
    return PeriodTable[fine * 36 + note];
}

uint8_t period_to_note(const uint16_t period, const uint8_t fine)
{
    const int ofs = fine * 36;
    for (int i = 0; i < 36; i++) {
        if ((period >= PeriodTable[ofs + i])) {
            return i;
        }
    }
    return 35;
}

}  // namespace


void FT_Player::ft_play_voice(int pat, const int chn)
{
    const uint32_t event = mdata.read32b(1084 + pat * 1024 + ft_pattern_pos * 16 + chn * 4);
    const uint16_t note = (event & 0xfff0000) >> 16;
    const uint8_t cmd = (event & 0x0f00) >> 8;
    const uint8_t cmdlo = event & 0xff;

    auto& ch = ft_chantemp[chn];

    const uint8_t n_cmd = ch.n_command >> 8;  // ax
    if (n_cmd == 0) {
        ch.output_period = ch.n_period;
    } else if (n_cmd == 4 || n_cmd == 6) {
        if (cmd != 4 && cmd != 6) {
            ch.output_period = ch.n_period;
        }
    }

    ch.n_command = event & 0xffff;

    const int insnum = ((event & 0xf0000000) >> 24) | ((cmd & 0xf0) >> 4);
    if (insnum) {
        const int ins = insnum - 1;
        const int ofs = 20 + 30 * (ins - 1) + 22;
        ch.n_insnum = insnum;
        ch.output_volume = ch.n_volume = mdata.read8(ofs + 3);
        ch.n_finetune = mdata.read8(ofs + 2);
    }

    if (cmd == 3 || cmd == 5) {   // check if tone portamento
        if (note) {
            // See period conversion comment below.
            ch.n_wantedperiod = note_to_period(period_to_note(note, 0), ch.n_finetune);
            if (note == ch.n_period) {
                ch.n_toneportdirec = 0;
            } else if (note < ch.n_period) {
                ch.n_toneportdirec = 2;
            } else {
                ch.n_toneportdirec = 1;
            }
        }
        if (cmd != 5) {
            if (cmdlo != 0) {
                ch.n_toneportspeed = cmdlo;
            }
        }
    } else {
        if (note != 0 && ch.n_insnum > 0) {    // add insnum sanity check
            if (cmd == 0x0e && (cmdlo & 0xf0) == 0x50) {    // check if set finetune
                ch.n_finetune = cmdlo & 0x0f;
            }

            // FT uses pre-converted periods to note numbers. We'll convert to note
            // numbers here and back to periods to set finetune. Note: "note" is
            // the event note which is actually a period value, not a note number.
            ch.n_period = note_to_period(period_to_note(note, 0), ch.n_finetune);
            if (cmd == 0x0e && (cmdlo & 0xf0) == 0xd0) {    // check if note delay
                if (cmdlo & 0x0f) {
                    return;
                }
            }

            const int ins = ch.n_insnum - 1;
            const int ofs = 20 + 30 * (ins - 1) + 22;

            int length = mdata.read16b(ofs);
            if (cmd == 9) {     // sample offset
                uint8_t val = cmdlo;
                if (val == 0) {
                    val = ch.n_offset;
                }
                ch.n_offset = val;

                const uint16_t l = val << 8;
                if (l > length) {
                    length = 0;
                } else {
                    length -= l;
                }
            }

            ch.n_length = length;
            ch.n_loopstart = mdata.read16b(ofs + 4);
            ch.n_replen = mdata.read16b(ofs + 6);
            ch.output_period = ch.n_period;

            if ((ch.n_wavecontrol & 0x04) == 0) {
                ch.n_vibratopos = 0;
            }
            if ((ch.n_wavecontrol & 0x40) == 0) {
                ch.n_tremolopos = 0;
            }
        }
    }

    if (cmd == 0 && cmdlo == 0) {
        return;
    }

    switch (cmd) {
    case 0x0c:
        ft_volume_change(chn, cmdlo);
        break;
    case 0x0e:
        ft_e_commands(chn, cmdlo);
        break;
    case 0x0b:
        ft_position_jump(cmdlo);
        break;
    case 0x0d:
        ft_pattern_break(cmdlo);
        break;
    case 0x0f:
        ft_set_speed(cmdlo);
        break;
    }
}

void FT_Player::ft_e_commands(const int chn, uint8_t cmdlo)
{
    switch (cmdlo >> 4) {
    case 0x1:
        ft_fine_porta_up(chn, cmdlo);
        break;
    case 0x2:
        ft_fine_porta_down(chn, cmdlo);
        break;
    case 0x3:
        ft_set_gliss_control(chn, cmdlo);
        break;
    case 0x4:
        ft_set_vibrato_control(chn, cmdlo);
        break;
    case 0x7:
        ft_set_tremolo_control(chn, cmdlo);
        break;
    case 0x9:
        ft_retrig_note(chn);
        break;
    case 0xa:
        ft_volume_fine_up(chn, cmdlo);
        break;
    case 0xb:
        ft_volume_fine_down(chn, cmdlo);
        break;
    case 0xc:
        ft_note_cut(chn, cmdlo);
        break;
    case 0x6:
        ft_jump_loop(chn, cmdlo);
        break;
    case 0xe:
        ft_pattern_delay(chn, cmdlo);
        break;
    }
}

void FT_Player::ft_position_jump(uint8_t cmdlo)
{
    ft_song_pos = --cmdlo;
    ft_pbreak_pos = 0;
    ft_pos_jump_flag = true;
    position_jump_cmd = true;
}

void FT_Player::ft_volume_change(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    if (cmdlo > 64) {
        cmdlo = 64;
    }
    ch.output_volume = cmdlo;
    ch.n_volume = cmdlo;
}

void FT_Player::ft_pattern_break(uint8_t cmdlo)
{
    const uint8_t row = (cmdlo >> 4) * 10 + (cmdlo & 0x0f);
    if (row <= 63) {
        // mt_pj2
        ft_pbreak_pos = row;
    }
    ft_pos_jump_flag = true;
}

void FT_Player::ft_set_speed(uint8_t cmdlo)
{
    if (cmdlo < 0x20) {
        ft_speed = cmdlo;
        ft_counter = cmdlo;
    } else {
        cia_tempo = cmdlo;
    }
}

void FT_Player::ft_fine_porta_up(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    int val = ch.n_period;
    val -= cmdlo & 0x0f;
    if (val < 113) {
        val = 113;
    }
    ch.n_period = val;
    ch.output_period = val;
}

void FT_Player::ft_fine_porta_down(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    int val = ch.n_period;
    val += cmdlo & 0x0f;
    if (val < 856) {
        val = 856;
    }
    ch.n_period = val;
    ch.output_period = val;
}

void FT_Player::ft_set_gliss_control(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    ch.n_gliss = (cmdlo & 0x0f) != 0;
}

void FT_Player::ft_set_vibrato_control(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    ch.n_wavecontrol &= 0xf0;
    ch.n_wavecontrol |= cmdlo & 0x0f;
}

void FT_Player::ft_jump_loop(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    cmdlo &= 0x0f;
    if (cmdlo == 0) {
        ch.n_pattpos = ft_pattern_pos;
    } else {
        if (ch.n_loopcount == 0) {
            ch.n_loopcount = cmdlo;
            ft_pbreak_pos = ch.n_pattpos;
            ft_pbreak_flag = true;
            ch.inside_loop = true;
        } else {
            ch.n_loopcount -= 1;
            if (ch.n_loopcount == 0) {
                ch.inside_loop = false;
            }
        }
    }
}

void FT_Player::ft_set_tremolo_control(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    ch.n_wavecontrol &= 0x0f;
    ch.n_wavecontrol |= (cmdlo & 0x0f) << 4;
}

void FT_Player::ft_retrig_note(const int chn)
{
    auto& ch = ft_chantemp[chn];
    const int ins = ch.n_insnum - 1;
    const int ofs = 20 + 30 * (ins - 1) + 22;
    ch.n_length = mdata.read16b(ofs);;
    ch.n_loopstart = mdata.read16b(ofs + 4);
    ch.n_replen = mdata.read16b(ofs + 6);
}

void FT_Player::ft_volume_fine_up(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    cmdlo &= 0x0f;
    cmdlo += ch.n_volume;
    if (cmdlo > 64) {
        cmdlo = 64;
    }
    ch.output_volume = cmdlo;
    ch.n_volume = cmdlo;
}

void FT_Player::ft_volume_fine_down(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    cmdlo &= 0x0f;
    if (cmdlo > ch.n_volume) {
        cmdlo = 0;
    } else {
        cmdlo = ch.n_volume - cmdlo;
    }
    ch.output_volume = cmdlo;
    ch.n_volume = cmdlo;
}

void FT_Player::ft_note_cut(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    if ((cmdlo & 0x0f) == 0) {
        ch.n_volume = 0;
        ch.output_volume = 0;
    }
}

void FT_Player::ft_pattern_delay(const int chn, uint8_t cmdlo)
{
    if (ft_patt_del_time_2 != 0) {
        return;
    }
    ft_patt_del_time = (cmdlo & 0x0f) + 1;
}

void FT_Player::ft_check_efx(const int chn)
{
    const uint8_t cmd = ((ft_chantemp[chn].n_command & 0x0f00) >> 8);
    uint8_t cmdlo = (ft_chantemp[chn].n_command & 0xff);

    if (cmd == 0 && cmdlo == 0) {
        return;
    }

    switch (cmd) {
    case 0xe:
        ft_more_e_commands(chn, cmdlo);
        break;
    case 0x0:
        ft_arpeggio(chn, cmdlo);
        break;
    case 0x1:
        ft_porta_up(chn, cmdlo);
        break;
    case 0x2:
        ft_porta_down(chn, cmdlo);
        break;
    case 0x3:
        ft_tone_portamento(chn);
        break;
    case 0x4:
        ft_vibrato(chn, cmdlo);
        break;
    case 0x5:
        ft_tone_plus_vol_slide(chn, cmdlo);
        break;
    case 0x6:
        ft_vibrato_plus_vol_slide(chn, cmdlo);
        break;
    case 0x7:
        ft_tremolo(chn, cmdlo);
        break;
    case 0xa:
        ft_volume_slide(chn, cmdlo);
        break;
    }
}

void FT_Player::ft_volume_slide(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    uint8_t val = cmdlo >> 4;
    if (val == 0) {
        val = cmdlo;
        if (val > ch.n_volume) {
            val = 0;
        } else {
            val = ch.n_volume - val;
        }
    } else {
        val += ch.n_volume;
        if (val > 64) {
            val = 64;
        }
    }
    ch.output_volume = val;
    ch.n_volume = val;
}

void FT_Player::ft_more_e_commands(const int chn, uint8_t cmdlo)
{
    switch (cmdlo >> 4) {
    case 0x9:
        ft_retrig_note_2(chn, cmdlo);
        break;
    case 0xc:
        ft_note_cut_2(chn, cmdlo);
        break;
    case 0xd:
        ft_note_delay_2(chn, cmdlo);
        break;
    }
}

void FT_Player::ft_arpeggio(const int chn, uint8_t cmdlo)
{
    if (cmdlo == 0) {
        return;
    }

    int val;
    switch (ArpeggioTable[ft_counter]) {
    case 0:
        val = 0;
        break;
    case 1:
        val = cmdlo >> 4;
        break;
    default:
        val = cmdlo & 0x0f;
        break;
    };

    auto& ch = ft_chantemp[chn];
    const uint16_t note = period_to_note(ch.n_period, ch.n_finetune) + val;
    if (note > 35) {
        ch.output_period = 0;
        return;
    }
    ft_chantemp[chn].output_period = note_to_period(note, ch.n_finetune);
}

void FT_Player::ft_porta_up(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    uint16_t val = ch.n_period - cmdlo;
    if (val < 113) {
        val = 113;
    }
    ch.n_period = val;
    ch.output_period = val;
}

void FT_Player::ft_porta_down(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    uint16_t val = ch.n_period + cmdlo;
    if (val > 856) {
        val = 856;
    }
    ch.n_period = val;
    ch.output_period = val;
}

void FT_Player::ft_tone_portamento(const int chn)
{
    auto& ch = ft_chantemp[chn];

    uint16_t val = ch.n_period;

    if (ch.n_toneportdirec > 1) {
        // porta up
        if (ch.n_toneportspeed > val) {
            val = 0;
        } else {
            val -= ch.n_toneportspeed;
        }
        if (val < ch.n_wantedperiod) {
            val = ch.n_wantedperiod;
            ch.n_toneportdirec = 1;
        }
    } else if (ch.n_toneportdirec != 1) {
        return;
    } else {
        // porta down
        val += ch.n_toneportspeed;
        if (val > ch.n_wantedperiod) {
            val = ch.n_wantedperiod;
            ch.n_toneportdirec = 1;
        }
    }

    ch.n_period = val;
    if (ch.n_gliss) {
        const uint16_t note = period_to_note(val, ch.n_finetune);
        val = note_to_period(note, ch.n_finetune);
    }
    ch.output_period = val;
}

void FT_Player::ft_vibrato(const int chn, uint8_t cmdlo)
{
    if (cmdlo != 0) {
        auto& ch = ft_chantemp[chn];
        const uint8_t depth = cmdlo & 0x0f;
        if (depth != 0) {
            ch.n_vibratodepth = depth;
        }
        const uint8_t speed = (cmdlo & 0xf0) >> 2;
        if (speed != 0) {
            ch.n_vibratospeed = speed;
        }
    }

    ft_vibrato_2(chn);
}

void FT_Player::ft_vibrato_2(const int chn)
{
    auto& ch = ft_chantemp[chn];
    int pos = (ch.n_vibratopos >> 2) & 0x1f;  // al

    int val;
    switch (ch.n_wavecontrol & 0x03) {
    case 0:  // sine
        val = VibratoTable[pos];
        break;
    case 1:  // rampdown
        pos <<= 3;
        val = (ch.n_vibratopos & 0x80) ? ~pos : pos;
        break;
    default:  // square
        val = 255;
    }

    int period = ch.n_period;
    const int amt = (val * ch.n_vibratodepth) >> 7;
    if ((ch.n_vibratopos & 0x80) == 0) {
        period += amt;
        if (period > amt) {
            period -= amt;
        }
    } else {
        period -= amt;
        if (period < 0) {
            period = 0;
        }
    }

    ch.output_period = period;
    ch.n_vibratopos += ch.n_vibratospeed;
}

void FT_Player::ft_tone_plus_vol_slide(const int chn, uint8_t cmdlo)
{
    ft_tone_portamento(chn);
    ft_volume_slide(chn, cmdlo);
}

void FT_Player::ft_vibrato_plus_vol_slide(const int chn, uint8_t cmdlo)
{
    ft_vibrato_2(chn);
    ft_volume_slide(chn, cmdlo);
}

void FT_Player::ft_tremolo(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];
    if (cmdlo != 0) {
        if ((cmdlo & 0x0f) != 0) {
             ch.n_tremolodepth = cmdlo & 0x0f;
        }
        if ((cmdlo & 0xf0) != 0) {
             ch.n_tremolospeed = cmdlo >> 4;
        }
    }

    int pos = (ch.n_tremolopos >> 2) & 0x1f;

    int val;
    switch ((ch.n_wavecontrol >> 4) & 0x03) {
    case 0:  // sine
        val = VibratoTable[pos];
        break;
    case 1:  // rampdown
        pos <<= 3;
        val = (ch.n_vibratopos & 0x80) ? ~pos : pos;  // <-- bug in FT code
        break;
    default:  // square
        val = 255;
    }

    int volume = ch.n_volume;
    const int amt = (val * ch.n_tremolodepth) >> 6;
    if ((ch.n_tremolopos & 0x80) == 0) {
        volume += amt;
        if (volume > 64) {
            volume = 64;
        }
    } else {
        volume -= amt;
        if (volume < 0) {
            volume = 0;
        }
    }

    if ((ch.n_tremolopos & 0x80) == 0) {
        volume += amt;
        if (volume > 64) {
            volume = 64;
        }
    } else {
        volume -= amt;
        if (volume < 0) {
            volume = 0;
        }
    }

    ch.output_volume = volume;
    ch.n_tremolopos += ch.n_tremolospeed;
}

void FT_Player::ft_retrig_note_2(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];

    if (ft_speed - ft_counter == (cmdlo & 0x0f)) {
        const int ins = ch.n_insnum - 1;
        const int ofs = 20 + 30 * (ins - 1) + 22;
        ch.n_length = mdata.read16b(ofs);;
        ch.n_loopstart = mdata.read16b(ofs + 4);
        ch.n_replen = mdata.read16b(ofs + 6);
        ch.output_period = ch.n_period;
    }
}

void FT_Player::ft_note_cut_2(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];

    if (ft_speed - ft_counter == (cmdlo & 0x0f)) {
        ch.output_volume = 0;
        ch.n_volume = 0;
    }
}

void FT_Player::ft_note_delay_2(const int chn, uint8_t cmdlo)
{
    auto& ch = ft_chantemp[chn];

    if (ft_speed - ft_counter == (cmdlo & 0x0f)) {
        const int ins = ch.n_insnum - 1;
        const int ofs = 20 + 30 * (ins - 1) + 22;
        ch.n_length = mdata.read16b(ofs);;
        ch.n_loopstart = mdata.read16b(ofs + 4);
        ch.n_replen = mdata.read16b(ofs + 6);
        ch.output_period = ch.n_period;
    }
}

void FT_Player::ft_new_row()
{
    if (ft_counter != 1) {
        return;
    }

    // ft_dskip
    ft_pattern_pos += 1;
    if (ft_patt_del_time != 0) {
        ft_patt_del_time_2 = ft_patt_del_time;
        ft_patt_del_time = 0;
    }

    // ft_dskc
    if (ft_patt_del_time_2 != 0) {
        ft_patt_del_time_2 -= 1;
        if (ft_patt_del_time_2 != 0) {
            ft_pattern_pos -= 1;
        }
    }

    // ft_dska
    if (ft_pbreak_flag) {
        ft_pbreak_flag = false;
        ft_pattern_pos = ft_pbreak_pos;
        //ft_pbreak_pos = 0;
    }

    // ft_nnpysk
    if (ft_pattern_pos >= 64) {
        ft_next_position();
    } else {
        ft_no_new_pos_yet();
    }
}

void FT_Player::ft_next_position()
{
    while (true) {
        ft_pattern_pos = ft_pbreak_pos;
        ft_pbreak_pos = 0;
        ft_pos_jump_flag = false;

        uint8_t pos = ft_song_pos + 1;
        const uint8_t length = mdata.read8(950);
        const uint8_t restart = mdata.read8(951);
        if (pos >= length) {
            if (restart < length) {
                pos = restart;
            } else {
                pos = 0;
            }
        }
        ft_song_pos = pos;
        //ft_current_pattern = module.pattern_in_position(pos);
        if (!ft_pos_jump_flag) {
            return;
        }
    }
}

void FT_Player::ft_no_new_pos_yet()
{
    if (ft_pos_jump_flag) {
        ft_next_position();
    }
}

void FT_Player::ft_music()
{
    ft_counter -= 1;
    if (ft_counter == 0) {
        ft_counter = ft_speed;
        if (ft_patt_del_time_2 == 0) {
            ft_get_new_note();
        } else {
            ft_no_new_all_channels();
        }
    } else {
        ft_no_new_all_channels();
    }
    ft_new_row();
}

void FT_Player::ft_no_new_all_channels()
{
    for (int chn = 0; chn < channels; chn++) {
        ft_check_efx(chn);
    }
}

void FT_Player::ft_get_new_note()
{
    const int pat = mdata.read8(952 + ft_song_pos);

    for (int chn = 0; chn < channels; chn++) {
        ft_play_voice(pat, chn);
    }
}


namespace {

const uint8_t ArpeggioTable[32] = {
      0,   1,   2,   0,   1,   2,   0,   1,
      2,   0,   1,   2,   0,   1,   2,   0,
      0,  24,  49,  74,  97, 120, 141, 161,     // buffer overflow values (vibrato table)
    180, 197, 212, 224, 235, 244, 250, 253
};

const uint8_t VibratoTable[32] = {
      0,  24,  49,  74,  97, 120, 141, 161,
    180, 197, 212, 224, 235, 244, 250, 253,
    255, 253, 250, 244, 235, 224, 212, 197,
    180, 161, 141, 120,  97,  74,  49,  24
};

const uint16_t PeriodTable[16 * 36] = {
// Tuning 0, Normal
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
// Tuning 1
    850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450,
    425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 239, 225,
    213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113,
// Tuning 2
    844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447,
    422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237, 224,
    211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112,
// Tuning 3
    838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444,
    419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235, 222,
    209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111,
// Tuning 4
    832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441,
    416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233, 220,
    208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110,
// Tuning 5
    826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437,
    413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232, 219,
    206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109,
// Tuning 6
    820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434,
    410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230, 217,
    205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109,
// Tuning 7
    814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431,
    407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228, 216,
    204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108,
// Tuning -8
    907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,
    453, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
    226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
// Tuning -7
    900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477,
    450, 425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 238,
    225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119,
// Tuning -6
    894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474,
    447, 422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237,
    223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118,
// Tuning -5
    887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470,
    444, 419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235,
    222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118,
// Tuning -4
    881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467,
    441, 416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233,
    220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117,
// Tuning -3
    875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463,
    437, 413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232,
    219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116,
// Tuning -2
    868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460,
    434, 410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230,
    217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115,
// Tuning -1
    862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457,
    431, 407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228,
    216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114
};

}  // namespace

