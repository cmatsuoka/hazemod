#include "doctest.h"
#include "util/util.cpp"


TEST_SUITE("util") {
    TEST_CASE("util::string_format") {
        CHECK(string_format("") == "");
        CHECK(string_format("%d", 5000) == "5000");
        CHECK(string_format("abc%dxyz", 5000) == "abc5000xyz");
        CHECK(string_format("%s%d%3.2f", "abc", 5000, 3.21) == "abc50003.21");
        CHECK(string_format("") == "");
    }
}
