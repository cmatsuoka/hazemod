#include "doctest.h"
#include "util/databuffer.h"


TEST_SUITE("databuffer") {
    TEST_CASE("databuffer::check_buffer_size") {
        DataBuffer d(nullptr, 10);
        CHECK_NOTHROW(d.check_buffer_size(1));
        CHECK_NOTHROW(d.check_buffer_size(10));
        CHECK_THROWS_AS(d.check_buffer_size(0), std::runtime_error);
        CHECK_THROWS_AS(d.check_buffer_size(11), std::runtime_error);
    }

    TEST_CASE("databuffer::read8") {
        uint8_t b[] = { 1, 2, 3, 4, 5, 6, 7, 255 };
        DataBuffer d(b, 8);
        CHECK(d.read8(0) == 1);
        CHECK(d.read8(7) == 255);
        CHECK_THROWS_AS(d.read8(-1), std::runtime_error);
        CHECK_THROWS_AS(d.read8(8), std::runtime_error);
    }

    TEST_CASE("databuffer::read8i") {
        uint8_t b[] = { 1, 2, 3, 4, 5, 6, 7, 255 };
        DataBuffer d(b, 8);
        CHECK(d.read8i(0) == 1);
        CHECK(d.read8i(7) == -1);
        CHECK_THROWS_AS(d.read8(-1), std::runtime_error);
        CHECK_THROWS_AS(d.read8(8), std::runtime_error);
    }

    TEST_CASE("databuffer::read16b") {
        uint8_t b[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        DataBuffer d(b, 8);
        CHECK(d.read16b(0) == 0x0102);
        CHECK(d.read16b(1) == 0x0203);
        CHECK(d.read16b(6) == 0x0708);
        CHECK_THROWS_AS(d.read16b(7), std::runtime_error);
    }

    TEST_CASE("databuffer::read16l") {
        uint8_t b[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        DataBuffer d(b, 8);
        CHECK(d.read16l(0) == 0x0201);
        CHECK(d.read16l(1) == 0x0302);
        CHECK(d.read16l(6) == 0x0807);
        CHECK_THROWS_AS(d.read16l(7), std::runtime_error);
    }

    TEST_CASE("databuffer::read32b") {
        uint8_t b[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        DataBuffer d(b, 8);
        CHECK(d.read32b(0) == 0x01020304);
        CHECK(d.read32b(1) == 0x02030405);
        CHECK(d.read32b(4) == 0x05060708);
        CHECK_THROWS_AS(d.read32b(5), std::runtime_error);
    }

    TEST_CASE("databuffer::read32l") {
        uint8_t b[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        DataBuffer d(b, 8);
        CHECK(d.read32l(0) == 0x04030201);
        CHECK(d.read32l(1) == 0x05040302);
        CHECK(d.read32l(4) == 0x08070605);
        CHECK_THROWS_AS(d.read32l(5), std::runtime_error);
    }

    TEST_CASE("databuffer::read_string") {
        uint8_t b[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
        DataBuffer d(b, 8);
        CHECK(d.read_string(0, 3) == "abc");
        CHECK(d.read_string(1, 3) == "bcd");
        CHECK(d.read_string(5, 3) == "fgh");
        CHECK_THROWS_AS(d.read_string(5, 4), std::runtime_error);
    }

}
