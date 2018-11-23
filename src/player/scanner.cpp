#include "player/scanner.h"
#include "player/player.h"
#include "util/debug.h"

Scanner::Scanner() :
    end_point(0)
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

    player->start();

    haze::FrameInfo fi;

    while (true) {
        player->frame_info(fi);
        const int pos = fi.pos;
        const int row = fi.row;

        if (prev_row != row || prev_pos != pos || prev_loop_count != fi.loop_count) {

            Debug("scan: check %d/%d", pos, row);
            if (scan_cnt[pos][row] > 0) {
                if (player->inside_loop_) {
                    Debug("inside loop");
                } else {
                    Debug("scan: already visited");
                    break;
                }
            }

            scan_cnt[pos][row]++;
            prev_loop_count = fi.loop_count;
            prev_row = row;

            if (prev_pos != pos && !ord_data[pos].used) {
                ord_data[pos].state = player->save_state();
                ord_data[pos].time = player->time_;
                prev_pos = pos;
                ord_data[pos].used = true;
                Debug("scan: pos %d: time %lf", pos, ord_data[pos].time);
            }
        }

        player->play();
    }

    player->total_time_ = fi.time;
    Debug("end position is %d/%d", fi.pos, fi.row);

    scan_data[fi.song].num = scan_cnt[fi.pos][fi.row];
    scan_data[fi.song].row = fi.row;
    scan_data[fi.song].ord = fi.pos;
    scan_data[fi.song].frame = fi.frame;
}
