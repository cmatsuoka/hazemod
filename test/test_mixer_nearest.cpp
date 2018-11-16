#include "doctest.h"
#include "mixer/nearest.h"

TEST_SUITE("interpolator_nearest") {
    TEST_CASE("name") {
        NearestInterpolator itp;
        CHECK(itp.name() == "nearest");
    }

    TEST_CASE("nearest") {
        NearestInterpolator itp;
        CHECK(itp.get(0) == 0);
        CHECK(itp.get(0x4000) == 0);
        CHECK(itp.get(0x8000) == 0);
        CHECK(itp.get(0xc000) == 0);

        int16_t x = 100;
        itp.put(x);
        CHECK(itp.get(0) == 100);
        CHECK(itp.get(0x4000) == 100);
        CHECK(itp.get(0x8000) == 100);
        CHECK(itp.get(0xc000) == 100);

        x = 0;
        itp.put(x);
        CHECK(itp.get(0) == 0);
        CHECK(itp.get(0x4000) == 0);
        CHECK(itp.get(0x8000) == 0);
        CHECK(itp.get(0xc000) == 0);
    }
}
