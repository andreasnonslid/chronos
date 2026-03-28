#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include "../src/stopwatch.hpp"

using namespace std::chrono;
using tp = steady_clock::time_point;
using dur = steady_clock::duration;

static tp epoch() { return tp{}; }
static tp at_ms(int ms) { return epoch() + milliseconds(ms); }

TEST_CASE("Stopwatch start/stop basics", "[stopwatch]") {
    Stopwatch sw;
    REQUIRE_FALSE(sw.is_running());
    REQUIRE(sw.elapsed(epoch()) == dur::zero());

    sw.start(at_ms(0));
    REQUIRE(sw.is_running());
    REQUIRE(sw.elapsed(at_ms(100)) == milliseconds(100));

    sw.stop(at_ms(250));
    REQUIRE_FALSE(sw.is_running());
    REQUIRE(sw.elapsed(at_ms(9999)) == milliseconds(250));
}

TEST_CASE("Stopwatch accumulated time across start/stop cycles", "[stopwatch]") {
    Stopwatch sw;
    sw.start(at_ms(0));
    sw.stop(at_ms(100));
    sw.start(at_ms(200));
    sw.stop(at_ms(350));
    REQUIRE(sw.elapsed(at_ms(500)) == milliseconds(250));
}

TEST_CASE("Stopwatch lap recording", "[stopwatch]") {
    Stopwatch sw;
    sw.start(at_ms(0));
    sw.lap(at_ms(100));
    sw.lap(at_ms(350));
    REQUIRE(sw.laps().size() == 2);
    REQUIRE(sw.laps()[0] == milliseconds(100));
    REQUIRE(sw.laps()[1] == milliseconds(250));
}

TEST_CASE("Stopwatch cumulative lap time", "[stopwatch]") {
    Stopwatch sw;
    sw.start(at_ms(0));
    REQUIRE(sw.cumulative() == dur::zero());
    sw.lap(at_ms(100));
    REQUIRE(sw.cumulative() == milliseconds(100));
    sw.lap(at_ms(350));
    REQUIRE(sw.cumulative() == milliseconds(350));
    sw.reset();
    REQUIRE(sw.cumulative() == dur::zero());
}

TEST_CASE("Stopwatch lap ignored when not running", "[stopwatch]") {
    Stopwatch sw;
    sw.lap(at_ms(100));
    REQUIRE(sw.laps().empty());
}

TEST_CASE("Stopwatch reset clears everything", "[stopwatch]") {
    Stopwatch sw;
    sw.start(at_ms(0));
    sw.lap(at_ms(50));
    sw.stop(at_ms(100));
    sw.reset();
    REQUIRE_FALSE(sw.is_running());
    REQUIRE(sw.elapsed(at_ms(200)) == dur::zero());
    REQUIRE(sw.laps().empty());
}

TEST_CASE("Stopwatch double-start is a no-op", "[stopwatch]") {
    Stopwatch sw;
    sw.start(at_ms(0));
    sw.start(at_ms(500));
    sw.stop(at_ms(100));
    REQUIRE(sw.elapsed(at_ms(0)) == milliseconds(100));
}

TEST_CASE("Stopwatch stop when not running is a no-op", "[stopwatch]") {
    Stopwatch sw;
    sw.stop(at_ms(100));
    REQUIRE_FALSE(sw.is_running());
    REQUIRE(sw.elapsed(at_ms(200)) == dur::zero());
}

TEST_CASE("Stopwatch lap split is correct after pause/resume", "[stopwatch]") {
    Stopwatch sw;
    sw.start(at_ms(0));
    sw.lap(at_ms(1000));
    sw.stop(at_ms(2000));
    sw.start(at_ms(5000));
    sw.lap(at_ms(6000));
    REQUIRE(sw.laps()[1] == milliseconds(2000));
    REQUIRE(sw.cumulative() == milliseconds(3000));
}

TEST_CASE("Stopwatch restore paused state", "[stopwatch]") {
    Stopwatch sw;
    sw.restore(seconds(45), false, at_ms(0));
    REQUIRE_FALSE(sw.is_running());
    REQUIRE(sw.elapsed(at_ms(0)) == seconds(45));
    REQUIRE(sw.elapsed(at_ms(5000)) == seconds(45));
}

TEST_CASE("Stopwatch restore running state", "[stopwatch]") {
    Stopwatch sw;
    sw.restore(seconds(10), true, at_ms(0));
    REQUIRE(sw.is_running());
    REQUIRE(sw.elapsed(at_ms(0)) == seconds(10));
    REQUIRE(sw.elapsed(at_ms(5000)) == seconds(15));
}

TEST_CASE("Stopwatch restore clears previous laps", "[stopwatch]") {
    Stopwatch sw;
    sw.start(at_ms(0));
    sw.lap(at_ms(1000));
    REQUIRE(sw.laps().size() == 1);
    sw.restore(seconds(5), false, at_ms(0));
    REQUIRE(sw.laps().empty());
}
