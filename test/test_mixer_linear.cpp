#include "doctest.h"
#include "mixer/linear.h"

TEST_SUITE("interpolator_linear") {
    TEST_CASE("name") {
        LinearInterpolator itp;
        CHECK(itp.name() == "linear");
    }

    TEST_CASE("linear") {
        LinearInterpolator itp;
        CHECK(itp.sample(0) == 0);
        CHECK(itp.sample(0x4000) == 0);
        CHECK(itp.sample(0x8000) == 0);
        CHECK(itp.sample(0xc000) == 0);

        int16_t x = 100;
        itp.add(x);
        CHECK(itp.sample(0) == 0);
        CHECK(itp.sample(0x4000) == 25);
        CHECK(itp.sample(0x8000) == 50);
        CHECK(itp.sample(0xc000) == 75);

	x = 0;
        itp.add(x);
        CHECK(itp.sample(0) == 100);
        CHECK(itp.sample(0x4000) == 75);
        CHECK(itp.sample(0x8000) == 50);
        CHECK(itp.sample(0xc000) == 25);
    }
}
