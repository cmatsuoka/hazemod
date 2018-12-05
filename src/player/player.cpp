#include "player/player.h"
#include "player/scanner.h"


namespace haze {

Player::Player(void *buf, const uint32_t size, int ch, int sr) :
    mdata(buf, size),
    tempo_(125.0f),
    time_(0.0f),
    tempo_factor_(1.0f),
    total_time_(0.0f),
    enable_loop_(false),
    loop_count_(0),
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
    end_point_ = scanner_->scan_data(0).num;
    total_time_ = scanner_->scan_data(0).time;
}

void Player::frame_info(FrameInfo& fi)
{
    fi.time = time_;
    fi.total_time = total_time_;
    fi.loop_count = loop_count_;
}

bool Player::fill(int16_t *buf, int size)
{
    return Player::fill_buffer_<int16_t>(buf, size);
}

bool Player::fill(float *buf, int size)
{
    return Player::fill_buffer_<float>(buf, size);
}

void Player::check_end_of_module()
{
    auto data = scanner_->scan_data(0);  // song
    FrameInfo fi;
    frame_info(fi);
    if (fi.pos == data.ord && fi.row == data.row && fi.frame == data.frame) {
        if (end_point_ == 0) {
            loop_count_++;
            end_point_ = data.num;
        }
        end_point_--;
    }
}

void Player::set_position(int pos)
{
    if (pos < length()) {
        restore_state(scanner_->ord_data(pos).state);
    }
}

void Player::reset()
{
    set_position(0);
}

}  // namespace haze
