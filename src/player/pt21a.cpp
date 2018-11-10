#include <player/pt21a.h>


PT21A_Player::PT21A_Player(Mixer& mixer) :
    Player(
        "pt2",
        "Protracker V2.1A playroutine + fixes",
        "A player based on the Protracker V2.1A replayer + V2.3D fixes",
        "Claudio Matsuoka",
        { "m.k." },
        mixer
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
}
