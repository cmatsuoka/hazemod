#include "doctest.h"
#include "mixer/sample.cpp"

TEST_SUITE("sample") {
    TEST_CASE("sample::8bit") {
        Sample sample1;
        CHECK(sample1.size() == 0);
        CHECK(sample1.rate() == 1.0);
        CHECK(sample1.get(0) == 0);

        uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        Sample sample2(data, 8);
        CHECK(sample2.size() == 8);
        CHECK(sample2.rate() == 1.0);
        CHECK(sample2.get(0) == 256);
        CHECK(sample2.get(7) == 2048);
    }

    TEST_CASE("sample::16bit") {
        Sample sample1(Sample16Bits);
        CHECK(sample1.size() == 0);
        CHECK(sample1.rate() == 1.0);
        CHECK(sample1.get(0) == 0);

        int16_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        Sample sample2(data, 8, 1.0, Sample16Bits);
        CHECK(sample2.size() == 8);
        CHECK(sample2.rate() == 1.0);
        CHECK(sample2.get(0) == 1);
        CHECK(sample2.get(7) == 8);
    }

}
