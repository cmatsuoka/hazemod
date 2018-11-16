#include <stdexcept>
#include "doctest.h"
#include "mixer/voice.cpp"

TEST_SUITE("mixer_voice") {
    TEST_CASE("voice::set_interpolator") {
        Voice v(0, NearestInterpolatorType);
        v.set_interpolator(LinearInterpolatorType);
        CHECK_THROWS_AS(v.set_interpolator(SplineInterpolatorType), std::runtime_error);
    }

    TEST_CASE("voice::default_sample") {
        Voice v(0, NearestInterpolatorType);
        CHECK_NOTHROW(v.get());
    }

    TEST_CASE("voice::position") {
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        Sample s(data, 8);
        Voice v(0, NearestInterpolatorType);
        v.set_sample(s);
        v.set_start(0);
        CHECK(v.pos() == 0);
        CHECK(v.frac() == 0);
        v.set_voicepos(1.5);
        CHECK(v.pos() == 1);
        CHECK(v.frac() == 0x8000);
    }

    TEST_CASE("voice::sample") {
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        Sample s(data, 8, 1.0, Sample16Bits);
        Voice v(0, NearestInterpolatorType);
        v.set_sample(s);
        v.set_start(0);
        v.set_end(8);
        v.set_step(1.5);
        CHECK(v.get() == 0x10);
        CHECK(v.get() == 0x20);
        CHECK(v.get() == 0x40);
        CHECK(v.get() == 0x50);
        CHECK(v.get() == 0x70);
        CHECK(v.get() == 0x80);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
    }

    TEST_CASE("voice::sample_loop") {
        int16_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
        Sample s(data, 8, 1.0, Sample16Bits);
        Voice v(0, NearestInterpolatorType);
        v.set_sample(s);
        v.set_start(0);
        v.set_end(8);
        v.set_loop_start(2);
        v.set_loop_end(6);
        v.enable_loop(true);
        v.set_step(1.5);
        CHECK(v.get() == 0x10);
        CHECK(v.get() == 0x20);
        CHECK(v.get() == 0x40);
        CHECK(v.get() == 0x50);
        CHECK(v.get() == 0x30);
        CHECK(v.get() == 0x40);
        CHECK(v.get() == 0x60);
        CHECK(v.get() == 0x30);
        CHECK(v.get() == 0x50);
        CHECK(v.get() == 0x60);
    }
}
