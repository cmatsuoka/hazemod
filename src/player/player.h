#ifndef HAZE_PLAYER_PLAYER_H_
#define HAZE_PLAYER_PLAYER_H_

#include <string>
#include <vector>
#include "util/databuffer.h"
#include "util/options.h"


struct PlayerInfo {
    int pos;
    int row;
    int frame;
    int song;
    int speed;
    float tempo;
    float time;
};

class Player {
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

    Mixer& mixer;

public:
    Player(std::string const& id,
           std::string const& name,
           std::string const& description,
           std::string const& author,
           std::vector<std::string> accepts,
           Mixer& m,
           void *buf, const uint32_t size) :
        id_(id),
        name_(name),
        description_(description),
        author_(author),
        accepts_(accepts),
        speed_(6),
        tempo_(125.0f),
        time_(0.0f),
        initial_speed_(6),
        initial_tempo_(125.0f),
        mixer(m),
        mdata(buf, size)
    {
    }
    virtual void start() = 0;
    virtual void play() = 0;
    virtual void reset() = 0;
    virtual void info(PlayerInfo&) = 0;
};


#endif  // HAZE_PLAYER_PLAYER_H_
