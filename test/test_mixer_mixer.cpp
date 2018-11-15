#include "doctest.h"
#include "mixer/mixer.cpp"
#include <algorithm>
#include <iterator>

TEST_SUITE("mixer") {
    TEST_CASE("mix::1_channel") {
        Mixer m(1, 8287.0);
        int16_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        m.add_sample(data, 8, 1.0, Sample16Bits);
        m.set_sample(0, 0);
        m.set_period(0, 428.0);
        m.set_volume(0, 1024);
        m.set_start(0, 0);
        m.set_loop_start(0, 4);
        m.set_loop_end(0, 8);
        m.enable_loop(0, true);
        int16_t res[32];
        m.mix(res, 32);

        int16_t ref[] = {
            1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
            5, 5, 6, 6, 7, 7, 8, 8, 5, 5, 6, 6, 7, 7, 8, 8
        };

        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref)));
    }
}
