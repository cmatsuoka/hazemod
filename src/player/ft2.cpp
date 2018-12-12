#include "player/ft2.h"
#include "util/databuffer.h"


haze::Player *Ft2Player::new_player(void *buf, uint32_t size, std::string const& id, int sr)
{
    return new FT2_Player(buf, size, id, sr);
}

//----------------------------------------------------------------------

FT2_Player::FT2_Player(void *buf, uint32_t size, std::string const& id, int sr) :
    PCPlayer(buf, size, 32, sr)
{
    auto d = DataBuffer(buf, size);

    if (id == "xm") {
    } else {
    }
}

FT2_Player::~FT2_Player()
{
}

void FT2_Player::start()
{
    tempo_ = 125;
    time_ = 0.0f;

    const int pan = options_.get("pan", 70);
    const int panl = -128 * pan / 100;
    const int panr = 127 * pan / 100;

    mixer_->set_pan(0, panl);
    mixer_->set_pan(1, panr);
    mixer_->set_pan(2, panr);
    mixer_->set_pan(3, panl);
}

void FT2_Player::play()
{
    tempo_ = 125; //ft2play.tempo_;
    time_ += 20.0 * 125.0 / tempo_;
    inside_loop_ = ft2play.inside_loop_;
}

void FT2_Player::frame_info(haze::FrameInfo& fi)
{
#if 0
    fi.pos = ft2play.np_ord - 1;
    fi.row = ft2play.np_row;
    fi.num_rows = 64;
    fi.frame = ft2play.musiccount;
    fi.song = 0;
    fi.speed = ft2play.musicmax;
    fi.tempo = ft2play.tempo_;
#endif

    haze::Player::frame_info(fi);
}

State FT2_Player::save_state()
{
    return to_state<FT2_Player>(*this);
}

void FT2_Player::restore_state(State const& state)
{
    from_state<FT2_Player>(state, *this);
}

