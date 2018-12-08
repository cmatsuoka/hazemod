#include "doctest.h"
#include "format/format.h"


TEST_SUITE("format_format") {
    TEST_CASE("magic") {
        CHECK(MAGIC4('M', '.', 'K', '.') == 0x4d2e4b2e);
    }
}
