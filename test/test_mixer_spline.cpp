#include "doctest.h"
#include "mixer/spline.cpp"

TEST_SUITE("interpolator_spline") {
    TEST_CASE("name") {
        SplineInterpolator itp;
        CHECK(itp.name() == "spline");
    }

    TEST_CASE("spline") {
        SplineInterpolator itp;
        CHECK(itp.get(0) == 0);
        CHECK(itp.get(0x4000) == 0);
        CHECK(itp.get(0x8000) == 0);
        CHECK(itp.get(0xc000) == 0);

        int16_t x = 100;
        itp.put(x);
        CHECK(itp.get(0) == 0);
        CHECK(itp.get(0x4000) == -8);
        CHECK(itp.get(0x8000) == -7);
        CHECK(itp.get(0xc000) == -3);

	x = 0;
        itp.put(x);
        CHECK(itp.get(0) == 100);
        CHECK(itp.get(0x4000) == 86);
        CHECK(itp.get(0x8000) == 56);
        CHECK(itp.get(0xc000) == 22);
    }
}
