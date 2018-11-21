#ifndef HAZE_PLAYER_AMIGA_PLAYER_H_
#define HAZE_PLAYER_AMIGA_PLAYER_H_

#include "player/player.h"
#include "mixer/paula.h"


class AmigaPlayer : public haze::Player {

protected:
    Paula *paula_;

public:
    AmigaPlayer(void *buf, const uint32_t size, int sr) :
        haze::Player(buf, size, 4, sr)
    {
        paula_ = new Paula(buf, size, sr);
        haze::Player::mixer_ = static_cast<Mixer *>(paula_);
    }

    virtual ~AmigaPlayer() {
        delete paula_;
    }

    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void frame_info(haze::FrameInfo&) = 0;

    void amigaDMA(uint32_t, uint32_t);
};


#endif  // HAZE_PLAYER_PLAYER_H_
