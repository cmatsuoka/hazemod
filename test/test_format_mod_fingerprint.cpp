#include "doctest.h"
#include "format/mod.cpp"
#include "test.h"


TEST_SUITE("format_mod_fingerprint") {
    TEST_CASE("protracker") {
        char *buf;
        int len;

        REQUIRE_NOTHROW(buf = load("data/anar16.mod", len));

        ModFormat m;
        haze::ModuleInfo mi;
        REQUIRE(m.probe(buf, len, mi));
        delete buf;

        CHECK(mi.creator == "Protracker");

    }

    TEST_CASE("noisetracker") {
        char *buf;
        int len;

        REQUIRE_NOTHROW(buf = load("data/blue damage.mod", len));

        ModFormat m;
        haze::ModuleInfo mi;
        REQUIRE(m.probe(buf, len, mi));
        delete buf;

        CHECK(mi.creator == "Noisetracker");

    }

}
