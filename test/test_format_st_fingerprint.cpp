#include "doctest.h"
#include "format/st.cpp"
#include "test.h"


TEST_SUITE("format_st_fingerprint") {
    TEST_CASE("soundtracker") {
        char *buf;
        int len;

        REQUIRE_NOTHROW(buf = load("data/awesome4.mod", len));

        StFormat m;
        haze::ModuleInfo mi;
        REQUIRE(m.probe(buf, len, mi));
        delete [] buf;

        CHECK(mi.creator == "Soundtracker");

    }

}
