#include "doctest.h"
#include <mixer/nearest.h>

TEST_SUITE("interpolator_nearest") {
    TEST_CASE("name") {
        NearestNeighbor itp;
        CHECK(itp.name() == "nearest");
    }

    TEST_CASE("nearest::8bit") {
        NearestNeighbor itp;
        CHECK(itp.sample(0) == 0);
        CHECK(itp.sample(0x4000) == 0);
        CHECK(itp.sample(0x8000) == 0);
        CHECK(itp.sample(0xc000) == 0);

        int8_t x = 100;
        itp.add8(x);
        CHECK(itp.sample(0) == 25600);
        CHECK(itp.sample(0x4000) == 25600);
        CHECK(itp.sample(0x8000) == 25600);
        CHECK(itp.sample(0xc000) == 25600);

        x = 0;
        itp.add8(x);
        CHECK(itp.sample(0) == 0);
        CHECK(itp.sample(0x4000) == 0);
        CHECK(itp.sample(0x8000) == 0);
        CHECK(itp.sample(0xc000) == 0);
    }

    TEST_CASE("nearest::16bit") {
        NearestNeighbor itp;
        CHECK(itp.sample(0) == 0);
        CHECK(itp.sample(0x4000) == 0);
        CHECK(itp.sample(0x8000) == 0);
        CHECK(itp.sample(0xc000) == 0);

        int16_t x = 100;
        itp.add(x);
        CHECK(itp.sample(0) == 100);
        CHECK(itp.sample(0x4000) == 100);
        CHECK(itp.sample(0x8000) == 100);
        CHECK(itp.sample(0xc000) == 100);

        x = 0;
        itp.add(x);
        CHECK(itp.sample(0) == 0);
        CHECK(itp.sample(0x4000) == 0);
        CHECK(itp.sample(0x8000) == 0);
        CHECK(itp.sample(0xc000) == 0);
    }
}
