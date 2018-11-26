#ifndef HAZE_PLAYER_PLAYER_REGISTRY_H_
#define HAZE_PLAYER_PLAYER_REGISTRY_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <haze.h>


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

#endif  // HAZE_PLAYER_PLAYER_REGISTRY_H_
