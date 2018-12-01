#include "player/st2.h"
#include "util/databuffer.h"

// Scream Tracker 2 player
//
// Based on st2play - A very accurate C port of Scream Tracker 2.xx's replayer
// written by Sergei "x0r" Kolzun.


haze::Player *St2Play::new_player(void *buf, uint32_t size, int sr)
{
    return new ST2_Player(buf, size, sr);
}

//----------------------------------------------------------------------

ST2_Player::ST2_Player(void *buf, uint32_t size, int sr) :
    PCPlayer(buf, size, 4, sr),
    srate_(sr)
{
    auto d = DataBuffer(buf, size);
    st2_init_tables();

    // we can't use pointers so we'll initialize by hand
    //context = st2_tracker_init();

    memset(&context, 0, sizeof(context));

    context.tempo = 0x60;
    context.global_volume = 64;
    context.sample_rate = sr;
    context.frames_per_tick = 1;
    context.current_frame = 1;

    int i;
    for (i = 0; i < 32; ++i) {
        context.samples[i].length = 0;
        context.samples[i].loop_start = 0;
        context.samples[i].loop_end = 0xffff;
        context.samples[i].volume = 0;
        context.samples[i].c2spd = 8448;
    }

    uint8_t num_pat = d.read8(33);
    for (i = 0; i < 128; i++) {
        if (d.read8(48 + 31 * 32 + i) >= num_pat) {
            break;
        }
    }
    length_ = i;

    load(&context, d);
}

ST2_Player::~ST2_Player()
{
}

void ST2_Player::start()
{
    tempo_ = 125.0;
    time_ = 0.0f;

    const int pan = options_.get("pan", 70);
    const int panl = -128 * pan / 100;
    const int panr = 127 * pan / 100;

    mixer_->set_pan(0, panl);
    mixer_->set_pan(1, panr);
    mixer_->set_pan(2, panr);
    mixer_->set_pan(3, panl);

    st2_tracker_start(&context, srate_);
    st2_set_position(&context, 0);
}

void ST2_Player::play()
{
    st2_process_tick(&context);

    for (int chn = 0; chn < 4; chn++) {
        auto& ch = context.channels[chn];
        if (ch.trigger) {
            mixer_->set_sample(chn, ch.trigger);
            mixer_->set_loop_start(chn, ch.smp_loop_start);
            mixer_->set_loop_end(chn, ch.smp_loop_end);
            mixer_->enable_loop(chn, ch.smp_loop_start != 0xffff);
            ch.trigger = 0;
        }
        mixer_->set_period(chn, ch.period_current / FXMULT);
        mixer_->set_volume(chn, ch.volume_mix << 2);
    }

    tempo_factor_ = double(context.sample_rate) / context.frames_per_tick;
    tempo_ = 2.5 * tempo_factor_;
    time_ += 20.0 * 125.0 / tempo_;
}

int ST2_Player::length()
{
    return length_;
}

void ST2_Player::frame_info(haze::FrameInfo& fi)
{
    fi.pos = context.order_current;
    fi.row = context.channels[0].row;
    fi.num_rows = 64;
    fi.frame = context.ticks_per_row - context.current_tick - 1;
    fi.song = 0;
    fi.speed = context.ticks_per_row;
    fi.tempo = context.tempo;

    haze::Player::frame_info(fi);
}

State ST2_Player::save_state()
{
    return to_state<ST2_Player>(*this);
}

void ST2_Player::restore_state(State const& state)
{
    from_state<ST2_Player>(state, *this);
}

//----------------------------------------------------------------------

// from Sergei Kolzun's stmload.c
void ST2_Player::load(st2_context_t *ctx, DataBuffer const& d)
{
    stm_header_t stm;

    uint8_t code;
    int i, j;

    stm.version = 100 * d.read8(30) + d.read8(31);
    stm.tempo = d.read8(32);
    if (stm.version < 221) {
        stm.tempo = (stm.tempo / 10 << 4) + stm.tempo % 10;
    }
    ctx->tempo = stm.tempo;

    stm.patterns = d.read8(33);

    // TODO: song_init
    ctx->order_list_ptr = (uint8_t *)(malloc(128));
    ctx->pattern_data_ptr = (uint8_t *)(malloc(65536));
    // TODO: song_init

    stm.gvol = d.read8(34);
    if(stm.version > 210) {
        ctx->global_volume = stm.gvol;
    }

    int ofs = 48;
    for (i = 1; i < 32; ++i) {
        ctx->samples[i].id = d.read8(ofs + 12);
        ctx->samples[i].disk = d.read8(ofs + 13);
        ctx->samples[i].offset = d.read16l(ofs + 14);
        ctx->samples[i].length = d.read16l(ofs + 16);
        ctx->samples[i].loop_start = d.read16l(ofs + 18);
        ctx->samples[i].loop_end = d.read16l(ofs + 20);

        if (ctx->samples[i].loop_end == 0) {
            ctx->samples[i].loop_end = 0xffff;
        }

        ctx->samples[i].volume = d.read8(ofs + 22);
        ctx->samples[i].rsvd2 = d.read8(ofs + 23);
        ctx->samples[i].c2spd = d.read16l(ofs + 24);
        ctx->samples[i].rsvd3 = d.read32l(ofs + 26);
        ctx->samples[i].length_par = d.read16l(ofs + 30);

        // NON-ST2: amegas.stm has some samples with loop-point over the sample length.
        if (ctx->samples[i].loop_end != 0xffff && ctx->samples[i].loop_end > ctx->samples[i].length) {
            ctx->samples[i].loop_end = ctx->samples[i].length;
        }

        ofs += 32;
    }

    if (stm.version == 200) {
        i = 64;
    } else {
        i = 128;
    }

    ctx->order_list_ptr = d.ptr(ofs);
    ofs += i;

    for (i = 0; i < stm.patterns; ++i) {
        for (j = 0; j < 1024; ++j) {
            code = d.read8(ofs++);
            switch(code) {
            case 0xfb:
                ctx->pattern_data_ptr[(i << 10) + j] = 0; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0;
                break;
            case 0xfd:
                ctx->pattern_data_ptr[(i << 10) + j] = 0xfe; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0x01; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0x80; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0;
                break;
            case 0xfc:
                ctx->pattern_data_ptr[(i << 10) + j] = 0xff; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0x01; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0x80; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = 0;
                break;
            default:
                ctx->pattern_data_ptr[(i << 10) + j] = code; ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = d.read8(ofs++); ++j;
                code = ctx->pattern_data_ptr[(i << 10) + j] = d.read8(ofs++); ++j;
                ctx->pattern_data_ptr[(i << 10) + j] = d.read8(ofs++);
                if (stm.version < 221 && (code & 0x0f) == 1) {
                    code = ctx->pattern_data_ptr[(i << 10) + j];
                    ctx->pattern_data_ptr[(i << 10) + j] = (code / 10 << 4) + code % 10;
                }
            }
        }
    }

    for (i = 1; i < 32; ++i) {
        st2_sample_t *s = &ctx->samples[i];
        mixer_->add_sample(d.ptr(s->offset << 4), s->length,
            double(s->c2spd) / 8448.0);    // 8448 - 2.21; 8192 - 2.3
    }
}

//----------------------------------------------------------------------

