#ifndef HAZE_PLAYER_PLAYER_H_
#define HAZE_PLAYER_PLAYER_H_

#include <string>
#include <vector>
#include <haze.h>
#include "util/databuffer.h"
#include "util/options.h"


class Player : public haze::Player_ {
    std::string id_;
    std::string name_;
    std::string description_;
    std::string author_;
    std::vector<std::string> accepts_;

protected:
    DataBuffer mdata;
    Options options;

    int speed_;
    float tempo_;
    float time_;
    int initial_speed_;
    float initial_tempo_;

    int loop_count;
    int end_point;

    // TODO: scan_data
    bool inside_loop;

    Mixer *mixer_;

public:
    Player(std::string const& id,
           std::string const& name,
           std::string const& description,
           std::string const& author,
           std::vector<std::string> accepts,
           void *buf, const uint32_t size,
           int ch, int sr) :
        id_(id),
        name_(name),
        description_(description),
        author_(author),
        accepts_(accepts),
        mdata(buf, size),
        speed_(6),
        tempo_(125.0f),
        time_(0.0f),
        initial_speed_(6),
        initial_tempo_(125.0f)
    {
        mixer_ = new Mixer(ch, sr);
    }

    virtual ~Player() {
        delete mixer_;
    }

    Mixer *mixer() { return mixer_; }

    void fill(int16_t *buf, int size) override { mixer_->mix(buf, size); }
    void fill(float *buf, int size) override { mixer_->mix(buf, size); }
};


#endif  // HAZE_PLAYER_PLAYER_H_
