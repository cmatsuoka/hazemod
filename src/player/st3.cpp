#include "player/st3.h"
#include "util/databuffer.h"

// Scream Tracker 3 player
//
// Based on ST3PLAY v0.88 - 28th of November 2018 - https://16-bits.org
// St3play is a very accurate C port of Scream Tracker 3.21's replayer,
// by Olav "8bitbubsy" Sørensen, based on the original asm source codes
// by Sami "PSI" Tammilehto (Future Crew).
//
// Non-ST3 additions from other trackers (only handled for non ST3 S3Ms):
//
// - Mixing:
//   * 16-bit sample support
//   * Stereo sample support
//   * 2^31-1 sample length support
//   * Middle-C speeds beyond 65535
//   * Process the last 16 channels as PCM
//   * Process 8 octaves instead of 7
//   * The ability to play samples backwards
//
// - Effects:
//   * Command S2x        (set middle-C finetune)
//   * Command S5x        (panbrello type)
//   * Command S6x        (tick delay)
//   * Command S9x        (sound control - only S90/S91/S9E/S9F)
//   * Command Mxx        (set channel volume)
//   * Command Nxy        (channel volume slide)
//   * Command Pxy        (panning slide)
//   * Command Txx<0x20   (tempo slide)
//   * Command Wxy        (global volume slide)
//   * Command Xxx        (7+1-bit pan) + XA4 for 'surround'
//   * Command Yxy        (panbrello)
//   * Volume Command Pxx (set 4+1-bit panning)
//
// - Variables:
//   * Pan changed from 4-bit (0..15) to 8+1-bit (0..256)
//   * Memory variables for the new N/P/T/W/Y effects
//   * Panbrello counter
//   * Panbrello type
//   * Channel volume multiplier
//   * Channel surround flag
//
// - Other changes:
//   * Added tracker identification to make sure Scream Tracker 3.xx
//     modules are still played exactly like they should. :-)
//


haze::Player *St3Player::new_player(void *buf, uint32_t size, int sr)
{
    return new ST3_Player(buf, size, sr);
}

//----------------------------------------------------------------------

ST3_Player::ST3_Player(void *buf, uint32_t size, int sr) :
    PCPlayer(buf, size, 4, sr)
{
    auto d = DataBuffer(buf, size);
}

ST3_Player::~ST3_Player()
{
}

void ST3_Player::start()
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
}

void ST3_Player::play()
{
    //time_ += 20.0 * 125.0 / tempo_;
}

int ST3_Player::length()
{
    return 0;
}

void ST3_Player::frame_info(haze::FrameInfo& fi)
{
/*
    fi.pos = context.order_current;
    fi.row = context.channels[0].row;
    fi.num_rows = 64;
    fi.frame = context.ticks_per_row - context.current_tick - 1;
    fi.song = 0;
    fi.speed = context.ticks_per_row;
    fi.tempo = context.tempo;
*/

    haze::Player::frame_info(fi);
}

State ST3_Player::save_state()
{
    return to_state<ST3_Player>(*this);
}

void ST3_Player::restore_state(State const& state)
{
    from_state<ST3_Player>(state, *this);
}

//----------------------------------------------------------------------

void ST3_Player::load_s3m(DataBuffer const& d)
{
}

//----------------------------------------------------------------------
