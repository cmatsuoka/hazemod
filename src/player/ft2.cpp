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
    ft2play.ft2play_PlaySong(static_cast<const uint8_t *>(buf), size, false, false, sr);
    length_ = ft2play.song.len;
}

FT2_Player::~FT2_Player()
{
    ft2play.ft2play_Close();
}

void FT2_Player::start()
{
    tempo_ = 125;
    time_ = 0.0f;
}

void FT2_Player::play()
{
    ft2play.mainPlayer();

    tempo_ = ft2play.song.speed;
    time_ += 20.0 * 125.0 / tempo_;
    inside_loop_ = ft2play.inside_loop_;
}

void FT2_Player::frame_info(haze::FrameInfo& fi)
{
    fi.pos = ft2play.song.songPos;
    fi.row = ft2play.song.pattPos;
    fi.num_rows = ft2play.song.pattLen;
    fi.frame = ft2play.song.tempo - ft2play.song.timer;
    fi.song = 0;
    fi.speed = ft2play.song.tempo;
    fi.tempo = ft2play.song.speed;

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

