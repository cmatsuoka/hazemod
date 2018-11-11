#include "player/pt21a.h"


PT21A_Player::PT21A_Player(Mixer& mixer, void *buf, uint32_t size) :
    Player(
        "pt2",
        "Protracker V2.1A playroutine + fixes",
        "A player based on the Protracker V2.1A replayer + V2.3D fixes",
        "Claudio Matsuoka",
        { "m.k." },
        mixer,
        buf, size
    ),
    mt_SampleStarts{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    mt_SongDataPtr(0),
    mt_speed(6),
    mt_counter(0),
    mt_SongPos(0),
    mt_PBreakPos(0),
    mt_PosJumpFlag(false),
    mt_PBreakFlag(false),
    mt_LowMask(0),
    mt_PattDelTime(0),
    mt_PattDelTime2(0),
    mt_PatternPos(0)
{
}

void PT21A_Player::start()
{
    speed_ = 6;
    tempo_ = 125.0f;
    time_ = 0.0f;

    initial_speed_ = speed_;
    initial_tempo_ = tempo_;

    int num_pat = 0;
    for (int i = 0; i < 128; i++) {
        int pat = mdata.read8(952 + i);
        num_pat == std::max(num_pat, pat);
    }

    int offset = 1084 + 1024 * num_pat;
    for (int i = 0; i < 31; i++) {
        mt_SampleStarts[i] = offset;
        offset += mdata.read16b(20 + 22 * 30);
    }

    int pan = options.get("pan", 70);
    int panl = -128 * pan / 100;
    int panr = 127 * pan / 100;

    mixer.set_pan(0, panl);
    mixer.set_pan(1, panr);
    mixer.set_pan(2, panr);
    mixer.set_pan(3, panl);
}
