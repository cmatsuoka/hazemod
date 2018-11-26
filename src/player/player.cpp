#include "player/player.h"
#include "player/scanner.h"


namespace haze {

Player::Player(void *buf, const uint32_t size, int ch, int sr) :
    mdata(buf, size),
    tempo_(125.0f),
    time_(0.0f),
    tempo_factor_(1.0f),
    frame_size_(0),
    frame_remain_(0)
{
    scanner_ = new Scanner;
}

Player::~Player()
{
    delete scanner_;
}

void Player::scan()
{
    scanner_->scan(this);
}

void Player::frame_info(FrameInfo& pi)
{
    pi.time = time_;
    pi.total_time = total_time_;
}

void Player::fill(int16_t *buf, int size)
{
    Player::fill_buffer_<int16_t>(buf, size);
}

void Player::fill(float *buf, int size)
{
    Player::fill_buffer_<float>(buf, size);
}

}  // namespace haze
