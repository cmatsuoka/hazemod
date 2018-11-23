#include "doctest.h"
#include "util/state.h"
#include <string>
#include <array>

class Test {
    int a_;
    std::string b_;
    std::array<int, 1> c_;

public:
    Test(int a, std::string b, int c) : a_(a), b_(b), c_{0} { c_[0] = c; }
    void update(int a, std::string b, int c ) { a_ = a; b_ = b; c_[0] = c; }
    bool compare(int a, std::string b, int c ) { return a_ == a && b_ == b && c_[0] == c; }
    State save_state() { return to_state<Test>(*this); }
    void restore_state(State state) { from_state<Test>(state, *this); }
};


TEST_SUITE("util_state") {
    TEST_CASE("state_save_restore") {
        Test t(2, "giraffe", 3);
        REQUIRE(t.compare(2, "giraffe", 3));
        State x = t.save_state();
        t.update(5, "elephant", 6);
        REQUIRE(t.compare(5, "elephant", 6));
        t.restore_state(x);
        REQUIRE(t.compare(2, "giraffe", 3));
    }
}
