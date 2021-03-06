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
        mixer_ = new SoftMixer(ch, sr, options_);
    }

    virtual ~PCPlayer() {
        delete mixer_;
    }

    Mixer *mixer() override { return mixer_; }
};


#endif  // HAZE_PLAYER_PLAYER_H_
