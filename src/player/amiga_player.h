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
    }

    virtual ~AmigaPlayer() {
        delete paula_;
    }

    virtual void start() override = 0;
    virtual void play() override = 0;
    virtual void reset() override = 0;
    virtual int length() override = 0;
    virtual void frame_info(haze::FrameInfo&) override = 0;
    virtual State save_state() override = 0;
    virtual void restore_state(State const&) override = 0;

    Mixer *mixer() override { return paula_; }
};


#endif  // HAZE_PLAYER_PLAYER_H_
