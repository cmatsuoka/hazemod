#include "doctest.h"
#include "mixer/mixer.cpp"
#include <algorithm>
#include <iterator>

TEST_SUITE("mixer") {
    TEST_CASE("mix::1_channel") {
        Mixer m(1, 8287.0);
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        m.add_sample(data, 8, 1.0, Sample16Bits);
        m.set_sample(0, 0);
        m.set_period(0, 428.0);
        m.set_volume(0, 1024);
        m.set_start(0, 0);
        m.set_end(0, 8);
        m.set_loop_start(0, 4);
        m.set_loop_end(0, 8);
        m.enable_loop(0, true);
        int16_t res[32];
        m.mix(res, 32);

        int16_t ref[] = {
            0x08, 0x08, 0x10, 0x10, 0x18, 0x18, 0x20, 0x20,
            0x28, 0x28, 0x30, 0x30, 0x38, 0x38, 0x40, 0x40,
            0x28, 0x28, 0x30, 0x30, 0x38, 0x38, 0x40, 0x40,
            0x28, 0x28, 0x30, 0x30, 0x38, 0x38, 0x40, 0x40
        };

for (int i = 0; i < 32; i++) printf("%d %d\n", res[i], ref[i]);
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref)));
    }

    TEST_CASE("mix::small_buffer") {
        Mixer m(1, 8287.0);
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        m.add_sample(data, 8, 1.0, Sample16Bits);
        m.set_sample(0, 0);
        m.set_period(0, 428.0);
        m.set_volume(0, 1024);
        m.set_start(0, 0);
        m.set_end(0, 8);
        m.set_loop_start(0, 4);
        m.set_loop_end(0, 8);
        m.enable_loop(0, true);

        int16_t res[6];
        m.mix(res, 6);
        int16_t ref1[] = { 0x08, 0x08, 0x10, 0x10, 0x18, 0x18 };
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref1)));

        int16_t ref2[] = { 0x20, 0x20, 0x28, 0x28, 0x30, 0x30 };
        m.mix(res, 6);
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref2)));
    }
}
