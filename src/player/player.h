#ifndef HAZE_PLAYER_PLAYER_H_
#define HAZE_PLAYER_PLAYER_H_

#include <haze.h>
#include "mixer/mixer.h"
#include "util/databuffer.h"
#include "util/options.h"
#include "util/state.h"


class Scanner;

namespace haze {

class Player {

protected:
    DataBuffer mdata;
    Options options_;

    double tempo_;
    double time_;
    double tempo_factor_;
    double total_time_;

    int32_t frame_size_;
    int32_t frame_remain_;

    int loop_count_;
    bool inside_loop_;

    Scanner *scanner_;

    template<typename T> void fill_buffer_(T *buf, int32_t size) {
        size = size / (sizeof(T) * 2);  // stereo
        while (size > 0) {
            if (frame_remain_ == 0) {
                play();
                frame_remain_ = double(mixer()->rate()) * PalRate / (tempo_factor_ * tempo_ * 100.0);
            }
            int32_t to_fill = std::min(size, frame_remain_);
            mixer()->mix(buf, to_fill * 2);
            size -= to_fill;
            frame_remain_ -= to_fill;
            buf += to_fill * 2;
        }
    }

    bool check_end_of_module();

    friend class ::Scanner;

public:
    Player(void *, const uint32_t, int, int);
    virtual ~Player();

    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void frame_info(FrameInfo&);
    virtual int length() = 0;
    virtual Mixer *mixer() = 0;
    virtual State save_state() = 0;
    virtual void restore_state(State const&) = 0;

    uint32_t frame_size() { return frame_size_; }
    void scan();
    void fill(int16_t *buf, int size);
    void fill(float *buf, int size);
};

}  // namespace haze

#endif  // HAZE_PLAYER_PLAYER_H_
