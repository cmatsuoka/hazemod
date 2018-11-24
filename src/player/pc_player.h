#ifndef HAZE_PLAYER_PC_PLAYER_H_
#define HAZE_PLAYER_PC_PLAYER_H_

#include "player/player.h"
#include "mixer/softmixer.h"


class PCPlayer : public haze::Player {
protected:
    SoftMixer *mixer_;

public:
    PCPlayer(void *buf, const uint32_t size, int ch, int sr) :
        haze::Player(buf, size, ch, sr)
    {
        mixer_ = new SoftMixer(ch, sr);
    }

    virtual ~PCPlayer() {
        delete mixer_;
    }

    virtual void start() override = 0;
    virtual void play() override = 0;
    virtual void reset() override = 0;
    virtual int length() override = 0;
    virtual void frame_info(haze::FrameInfo&) override = 0;
    virtual State save_state() override = 0;
    virtual void restore_state(State const&) override = 0;

    Mixer *mixer() override { return mixer_; }
};


#endif  // HAZE_PLAYER_PLAYER_H_
