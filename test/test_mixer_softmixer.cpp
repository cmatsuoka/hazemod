#include "doctest.h"
#include "mixer/softmixer.cpp"
#include <algorithm>
#include <iterator>
#include "util/options.h"


TEST_SUITE("softmixer") {
    TEST_CASE("mix::1_channel") {
        SoftMixer m(1, 8287.0, Options{{"interpolator", "nearest"}});
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
            0x02, 0x02, 0x04, 0x04, 0x06, 0x06, 0x08, 0x08,
            0x0a, 0x0a, 0x0c, 0x0c, 0x0e, 0x0e, 0x10, 0x10,
            0x0a, 0x0a, 0x0c, 0x0c, 0x0e, 0x0e, 0x10, 0x10,
            0x0a, 0x0a, 0x0c, 0x0c, 0x0e, 0x0e, 0x10, 0x10
        };

        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref)));
    }

    TEST_CASE("mix::small_buffer") {
        SoftMixer m(1, 8287.0, Options{{"interpolator", "nearest"}});
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        m.add_sample(data, 8, 1.0, Sample16Bits);
        m.set_sample(0, 0);
        m.set_period(0, 428.0);
        m.set_volume(0, 256);
        m.set_start(0, 0);
        m.set_end(0, 8);
        m.set_loop_start(0, 4);
        m.set_loop_end(0, 8);
        m.enable_loop(0, true);

        int16_t res[6];
        m.mix(res, 6);
        int16_t ref1[] = { 0x02, 0x02, 0x04, 0x04, 0x06, 0x06 };
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref1)));

        int16_t ref2[] = { 0x08, 0x08, 0x0a, 0x0a, 0x0c, 0x0c };
        m.mix(res, 6);
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref2)));
    }

    TEST_CASE("mix::volume") {
        SoftMixer m(1, 8287.0, Options{{"interpolator", "nearest"}});
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        m.add_sample(data, 8, 1.0, Sample16Bits);
        m.set_period(0, 428.0);

        int16_t res[16];

        m.set_sample(0, 0);
        m.set_start(0, 0);
        m.set_end(0, 8);
        m.set_volume(0, 0);
        m.mix(res, 16);
        int16_t ref1[] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref1)));

        m.set_sample(0, 0);
        m.set_start(0, 0);
        m.set_end(0, 8);
        m.set_volume(0, 128);
        m.mix(res, 16);
        int16_t ref2[] = {
            0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04,
            0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08
        };
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref2)));
    }

    TEST_CASE("mix::pan") {
        SoftMixer m(1, 8287.0, Options{{"interpolator", "nearest"}});
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        m.add_sample(data, 8, 1.0, Sample16Bits);
        m.set_volume(0, 256);
        m.set_period(0, 428.0);

        int16_t res[16];

        m.set_sample(0, 0);
        m.set_start(0, 0);
        m.set_end(0, 8);
        m.set_pan(0, -64);
        m.mix(res, 16);
        int16_t ref1[] = {
            0x03, 0x01, 0x06, 0x02, 0x09, 0x03, 0x0c, 0x04,
            0x0f, 0x05, 0x12, 0x06, 0x15, 0x07, 0x18, 0x08
        };
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref1)));

        m.set_sample(0, 0);
        m.set_start(0, 0);
        m.set_end(0, 8);
        m.set_pan(0, 64);
        m.mix(res, 16);
        int16_t ref2[] = {
            0x01, 0x03, 0x02, 0x06, 0x03, 0x09, 0x04, 0x0c,
            0x05, 0x0f, 0x06, 0x12, 0x07, 0x15, 0x08, 0x18
        };
        CHECK(std::equal(std::begin(res), std::end(res), std::begin(ref2)));
    }
}
