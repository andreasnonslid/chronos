#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include "../src/timer.hpp"

using namespace std::chrono;
using tp = steady_clock::time_point;
using dur = steady_clock::duration;

static tp epoch() { return tp{}; }
static tp at_ms(int ms) { return epoch() + milliseconds(ms); }

TEST_CASE("Timer set and start counts down", "[timer]") {
    Timer t;
    t.set(seconds(10));
    t.start(at_ms(0));
    REQUIRE(t.is_running());
    REQUIRE(t.remaining(at_ms(3000)) == seconds(7));
}

TEST_CASE("Timer pause and resume", "[timer]") {
    Timer t;
    t.set(seconds(10));
    t.start(at_ms(0));
    t.pause(at_ms(3000));
    REQUIRE_FALSE(t.is_running());
    REQUIRE(t.remaining(at_ms(9999)) == seconds(7));

    t.start(at_ms(5000));
    REQUIRE(t.remaining(at_ms(7000)) == seconds(5));
}

TEST_CASE("Timer expired returns true when time is up", "[timer]") {
    Timer t;
    t.set(seconds(5));
    REQUIRE_FALSE(t.expired(at_ms(0)));

    t.start(at_ms(0));
    REQUIRE_FALSE(t.expired(at_ms(3000)));
    REQUIRE(t.expired(at_ms(5000)));
    REQUIRE(t.expired(at_ms(6000)));
}

TEST_CASE("Timer remaining clamps to zero", "[timer]") {
    Timer t;
    t.set(seconds(1));
    t.start(at_ms(0));
    REQUIRE(t.remaining(at_ms(2000)) == dur::zero());
}

TEST_CASE("Timer reset clears state", "[timer]") {
    Timer t;
    t.set(seconds(10));
    t.start(at_ms(0));
    t.reset();
    REQUIRE_FALSE(t.is_running());
    REQUIRE_FALSE(t.touched());
    REQUIRE(t.remaining(at_ms(0)) == dur::zero());
}

TEST_CASE("Timer touched flag", "[timer]") {
    Timer t;
    REQUIRE_FALSE(t.touched());
    t.set(seconds(5));
    REQUIRE_FALSE(t.touched());
    t.start(at_ms(0));
    REQUIRE(t.touched());
    t.pause(at_ms(100));
    REQUIRE(t.touched());
}

TEST_CASE("Timer double-start is a no-op", "[timer]") {
    Timer t;
    t.set(seconds(10));
    t.start(at_ms(0));
    t.start(at_ms(5000));
    REQUIRE(t.remaining(at_ms(3000)) == seconds(7));
}

TEST_CASE("Timer pause when not running is a no-op", "[timer]") {
    Timer t;
    t.set(seconds(10));
    t.pause(at_ms(100));
    REQUIRE_FALSE(t.is_running());
    t.start(at_ms(0));
    REQUIRE(t.remaining(at_ms(1000)) == seconds(9));
}

TEST_CASE("Timer with zero target is not expired", "[timer]") {
    Timer t;
    t.start(at_ms(0));
    REQUIRE_FALSE(t.expired(at_ms(1000)));
}
