#ifndef HAZE_PLAYER_PLAYER_H_
#define HAZE_PLAYER_PLAYER_H_

#include <string>
#include <vector>
#include <algorithm>
#include <haze.h>
#include "mixer/mixer.h"
#include "util/databuffer.h"
#include "util/options.h"


class FormatPlayer {
    std::string id_;
    std::string name_;
    std::string description_;
    std::string author_;
    std::vector<std::string> accepts_;

public:
    FormatPlayer(std::string id, std::string name, std::string description,
                 std::string author, std::vector<std::string> accepts) :
        id_(id),
        name_(name),
        description_(description),
        author_(author),
        accepts_(accepts) {}

    virtual ~FormatPlayer();

    std::string const& id() const { return id_; }
    std::string const& name() const { return name_; }
    std::string const& description() const { return description_; }
    std::string const& author() const { return author_; }
    std::vector<std::string> const& accepts() const { return accepts_; }

    virtual haze::Player *new_player(void *, uint32_t, int) = 0;
};

/*
class PlayerRegistry {
    std::unordered_map<std::string, FormatPlayer&> map_;
public:
    PlayerRegistry();
    haze::Player build(void *, const uint32_t, int, int);
    //void put(FormatPlayer const& x) { map_[x.id()] = x; }
    //FormatPlayer const& get(std::string const& id) { return map_[id]; }
    std::vector<FormatPlayer> list() {
        std::vector<FormatPlayer> v;
        for (auto kv : map_) { v.push_back(kv.second); }
        return v;
    }
};
*/

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

    int32_t frame_size_;
    int32_t frame_remain_;

    int loop_count;
    int end_point;

    // TODO: scan_data
    bool inside_loop;

    Mixer *mixer_;

    template<typename T> void fill_buffer_(T *buf, int32_t size) {
        size = size / (sizeof(T) * 2);  // stereo
        while (size > 0) {
            if (frame_remain_ == 0) {
                play();
                frame_remain_ = double(mixer_->rate()) * PalRate / (tempo_factor_ * tempo_ * 100.0);
            }
            int32_t to_fill = std::min(size, frame_remain_);
            mixer_->mix(buf, to_fill * 2);
            size -= to_fill;
            frame_remain_ -= to_fill;
            buf += to_fill * 2;
        }
    }

public:
    Player(void *buf, const uint32_t size, int ch, int sr) :
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
        mixer_ = new Mixer(ch, sr);
    }

    virtual ~Player() {
        delete mixer_;
    }

    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void frame_info(FrameInfo&) = 0;

    Mixer *mixer() { return mixer_; }

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
