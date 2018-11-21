#include <stdexcept>
#include "doctest.h"
#include "mixer/paula_channel.h"

TEST_SUITE("mixer_paula_channel") {

    TEST_CASE("voice::stop") {
        int8_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x7f };
        DataBuffer d(data, 8);
        PaulaChannel v(d, 8287);
        v.set_start(0);
        v.set_length(2);  // length is given in 16-bit words!
        v.set_period(428);
        v.set_volume(1);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
    }

    TEST_CASE("voice::volume") {
        int8_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x7f };
        DataBuffer d(data, 8);
        PaulaChannel v(d, 8287);
        v.set_start(0);
        v.set_length(4);  // length is given in 16-bit words!
        v.set_period(428);
        v.set_volume(16);
        v.start_dma();
        CHECK(v.get() == 0x100);
        CHECK(v.get() == 0x200);
        CHECK(v.get() == 0x300);
        v.set_volume(64);
        CHECK(v.get() == 0x1000);
        CHECK(v.get() == 0x1400);
        v.set_volume(0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
    }

    TEST_CASE("voice::loop") {
        int8_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x7f };
        DataBuffer d(data, 8);
        PaulaChannel v(d, 8287);
        v.set_start(0);
        v.set_length(2);  // length is given in 16-bit words!
        v.set_period(428);
        v.set_volume(1);
        v.start_dma();
        CHECK(v.get() == 0x10);
        CHECK(v.get() == 0x20);
        CHECK(v.get() == 0x30);
        CHECK(v.get() == 0x40);
        CHECK(v.get() == 0x10);
        CHECK(v.get() == 0x20);
    }

    TEST_CASE("voice::full_loop") {
        int8_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x7f };
        DataBuffer d(data, 8);
        PaulaChannel v(d, 8287);
        v.set_start(0);
        v.set_length(4);  // length is given in 16-bit words!
        v.set_period(428);
        v.set_volume(1);
        v.start_dma();
        v.set_start(2);   // set loop after DMA started
        v.set_length(2);
        CHECK(v.get() == 0x10);
        CHECK(v.get() == 0x20);
        CHECK(v.get() == 0x30);
        CHECK(v.get() == 0x40);
        CHECK(v.get() == 0x50);
        CHECK(v.get() == 0x60);
        CHECK(v.get() == 0x70);
        CHECK(v.get() == 0x7f);
        CHECK(v.get() == 0x30);  // loop here to loop start
        CHECK(v.get() == 0x40);
        CHECK(v.get() == 0x50);
        CHECK(v.get() == 0x60);  // new loop end
        CHECK(v.get() == 0x30);
        CHECK(v.get() == 0x40);
    }

    TEST_CASE("voice::no_loop") {
        int8_t data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x7f };
        DataBuffer d(data, 8);
        PaulaChannel v(d, 8287);
        v.set_start(0);
        v.set_length(4);  // length is given in 16-bit words!
        v.set_period(428);
        v.set_volume(1);
        v.start_dma();
        v.set_start(0);
        v.set_length(1);
        CHECK(v.get() == 0x10);
        CHECK(v.get() == 0x20);
        CHECK(v.get() == 0x30);
        CHECK(v.get() == 0x40);
        CHECK(v.get() == 0x50);
        CHECK(v.get() == 0x60);
        CHECK(v.get() == 0x70);
        CHECK(v.get() == 0x7f);
        CHECK(v.get() == 0);  // single shot sample ended
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
        CHECK(v.get() == 0);
    }

}
