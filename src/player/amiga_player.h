#ifndef HAZE_PLAYER_AMIGA_PLAYER_H_
#define HAZE_PLAYER_AMIGA_PLAYER_H_

#include "player/player.h"
#include "mixer/paula.h"


class AmigaPlayer : public haze::Player {

protected:
    Paula *paula_;

public:
    AmigaPlayer(void *buf, const uint32_t size, int ch, int sr) :
        haze::Player(buf, size, ch, sr)
    {
        paula = new Paula(sr);
    }

    virtual ~Player() {
        delete paula_;
    }

    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void frame_info(FrameInfo&) = 0;

    void amigaDMA(uint32_t, uint32_t);
};

}  // namespace haze

#endif  // HAZE_PLAYER_PLAYER_H_
