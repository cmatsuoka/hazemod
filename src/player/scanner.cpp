#include "player/scanner.h"
#include "player/player.h"
#include "util/debug.h"

Scanner::Scanner()
{
}

Scanner::~Scanner()
{
}

void Scanner::scan(haze::Player *player)
{
    int prev_pos = 9999;
    int prev_row = 9999;
    int prev_loop_count = 9999;

    const int len = player->length();
    for (int i = 0; i < len; i++) {
        scan_cnt_.push_back(std::vector<uint32_t>(256));
    }
    ord_data_.resize(len);

    haze::FrameInfo fi;

    while (true) {
        player->frame_info(fi);
        const int pos = fi.pos;
        const int row = fi.row;

        if (prev_row != row || prev_pos != pos || prev_loop_count != fi.loop_count) {

            //Debug("scan: check %d/%d", pos, row);
            if (scan_cnt_[pos][row] > 0) {
                if (player->inside_loop_) {
                    //Debug("inside loop");
                } else {
                    Debug("scan: already visited");
                    break;
                }
            }

            scan_cnt_[pos][row]++;
            prev_loop_count = fi.loop_count;
            prev_row = row;

            if (prev_pos != pos && !ord_data_[pos].used) {
                ord_data_[pos].state = player->save_state();
                ord_data_[pos].time = player->time_;
                prev_pos = pos;
                ord_data_[pos].used = true;
                Debug("scan: pos %d: time %lf", pos, ord_data_[pos].time);
            }
        }

        player->play();
    }

    Debug("end position is %d/%d", fi.pos, fi.row);

    scan_data_[fi.song].num = scan_cnt_[fi.pos][fi.row];
    scan_data_[fi.song].row = fi.row;
    scan_data_[fi.song].ord = fi.pos;
    scan_data_[fi.song].frame = fi.frame;
    scan_data_[fi.song].time = fi.time;

    player->restore_state(ord_data_[0].state);
    player->mixer()->reset();
}
