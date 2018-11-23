#ifndef HAZE_PLAYER_SCANNER_H_
#define HAZE_PLAYER_SCANNER_H_

#include <haze.h>
#include <cstdint>
#include <vector>
#include "player/player.h"


struct ScanData {
    int ord;
    int row;
    int frame;
    int num;

    ScanData() : ord(0), row(0), frame(0), num(0) {}
};

struct OrdData {
    State state;
    double time;
    bool used;

    OrdData() : time(0.0f), used(false) {}
    ~OrdData() {}
};

class Scanner {
    int end_point;
    ScanData scan_data[haze::MaxSequences];
    std::vector<OrdData> ord_data;
    std::vector<std::vector<uint32_t>> scan_cnt;

public:
    Scanner();
    ~Scanner();
    void scan(haze::Player *);
};


#endif  // HAZE_PLAYER_SCAN_H_