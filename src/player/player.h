#ifndef HAZE_PLAYER_PLAYER_H_
#define HAZE_PLAYER_PLAYER_H_

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <haze.h>
#include "player/scanner.h"
#include "mixer/mixer.h"
#include "util/databuffer.h"
#include "util/options.h"


class FormatPlayer {
    haze::PlayerInfo info_;

public:
    FormatPlayer(std::string id, std::string name, std::string description,
                 std::string author, std::vector<std::string> formats) :
        info_{id, name, description, author, formats} {}

    virtual ~FormatPlayer() {}

    haze::PlayerInfo const& info() { return info_; }

    bool accepts(std::string const& fmt) {
        return std::find(info_.formats.begin(), info_.formats.end(), fmt) != info_.formats.end();
    }

    virtual haze::Player *new_player(void *, uint32_t, int) = 0;
};

class PlayerRegistry : public std::unordered_map<std::string, FormatPlayer *> {
public:
    PlayerRegistry();
    ~PlayerRegistry();
};


namespace haze {

class Player {

protected:
    DataBuffer mdata;
    Options options;

    int speed_;
    double tempo_;
    double time_;
    int initial_speed_;
    double initial_tempo_;
    double tempo_factor_;
    double total_time_;

    int32_t frame_size_;
    int32_t frame_remain_;

    int loop_count_;
    bool inside_loop_;

    Scanner scanner_;

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
    void scan();

    friend class ::Scanner;

public:
    Player(void *, const uint32_t, int, int);
    virtual ~Player();

    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void frame_info(FrameInfo&) = 0;
    virtual Mixer *mixer() = 0;
    virtual void *save_state() = 0;
    virtual void restore_state(void *) = 0;

    uint32_t frame_size() { return frame_size_; }

    void fill(int16_t *buf, int size) {
        fill_buffer_<int16_t>(buf, size);
    }

    void fill(float *buf, int size) {
        fill_buffer_<float>(buf, size);
    }
};

}  // namespace haze

#endif  // HAZE_PLAYER_PLAYER_H_
