#include "player/ft2.h"
#include <cmath>
#include "util/databuffer.h"


haze::Player *Ft2Player::new_player(void *buf, uint32_t size, std::string const& id, int sr)
{
    return new FT2_Player(buf, size, id, sr);
}

//----------------------------------------------------------------------

FT2_Player::FT2_Player(void *buf, uint32_t size, std::string const& id, int sr) :
    PCPlayer(buf, size, 32, sr)
{
    ft2play.mixer_ = mixer_;
    ft2play.ft2play_PlaySong(static_cast<const uint8_t *>(buf), size, false, false, sr);
    length_ = ft2play.song.len;
    freq_factor_ = 16384.0 * 1712.0 / sr * 8363.0;
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
    for (int i = 0; i < ft2play.song.antChn; i++) {
        ft2play::voice_t *v = &ft2play.voice[i];
        v->status = 0;
    }

    ft2play.mainPlayer();
    ft2play.mix_UpdateChannelVolPanFrq();

    for (int i = 0; i < ft2play.song.antChn; i++) {
        ft2play::voice_t *v = &ft2play.voice[i];
        if (v->status & ft2play::IS_NyTon) {
            mixer_->set_sample(i, v->smp);
            mixer_->set_end(i, v->SLen);
            mixer_->set_loop_start(i, v->SRepS);
            mixer_->set_loop_end(i, v->SRepS + v->SRepL);
            mixer_->set_voicepos(i, v->SPos);
            mixer_->enable_loop(i, (v->typ & 0x03) != 0, (v->typ & 0x02) != 0);
        }

        if (v->status & ft2play::IS_Period) {
            mixer_->set_period(i, v->SFrq > 0 ? freq_factor_ / v->SFrq : 0);
        }

        if (v->status & ft2play::IS_Vol) {
            mixer_->set_volume(i, v->SVol);
        }

        if (v->status & ft2play::IS_Pan) {
            mixer_->set_pan(i, int(v->SPan) - 128);
        }

        if (!v->mixRoutine) {
            mixer_->set_period(i, 0);
        }
    }

    inside_loop_ = false;
    for (int i = 0; i < ft2play.song.antChn; i++) {
        inside_loop_ |= ft2play.stm[i].inside_loop;
    }

    tempo_ = ft2play.song.speed;
    time_ += 20.0 * 125.0 / tempo_;
}

void FT2_Player::frame_info(haze::FrameInfo& fi)
{
    fi.pos = ft2play.song_pos_;
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

