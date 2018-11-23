#include "player/player.h"
#include "player/scanner.h"
#include "player/pt21a.h"
#include "player/nt11.h"
#include "player/st2.h"
#include "player/ust.h"


PlayerRegistry::PlayerRegistry()
{
    this->insert({
        { "pt2" , new Protracker2  },
        { "nt",   new Noisetracker },
        { "dst2", new DOC_Soundtracker_2 },
        { "ust",  new UltimateSoundtracker }
    });
}


PlayerRegistry::~PlayerRegistry()
{
    for (auto item : *this) {
        delete item.second;
    }
}


namespace haze {

Player::Player(void *buf, const uint32_t size, int ch, int sr) :
    mdata(buf, size),
    speed_(6),
    tempo_(125.0f),
    time_(0.0f),
    initial_speed_(6),
    initial_tempo_(125.0f),
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
}  // namespace haze
