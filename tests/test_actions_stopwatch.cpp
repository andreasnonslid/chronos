#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "../src/actions.hpp"
#include "../src/app.hpp"

using namespace std::chrono;
using sc = steady_clock;

static sc::time_point t0() { return sc::time_point{}; }
static sc::time_point at_ms(int ms) { return t0() + milliseconds(ms); }

// ─── stopwatch ───────────────────────────────────────────────────────────────

TEST_CASE("A_SW_START starts stopwatch and creates lap file path", "[actions]") {
    App app;
    REQUIRE_FALSE(app.sw.is_running());
    REQUIRE(app.sw_lap_file.empty());

    dispatch_action(app, A_SW_START, t0(), {});
    REQUIRE(app.sw.is_running());
    REQUIRE_FALSE(app.sw_lap_file.empty());
}

TEST_CASE("A_SW_START second call stops stopwatch", "[actions]") {
    App app;
    dispatch_action(app, A_SW_START, t0(), {});
    auto lap_file = app.sw_lap_file;
    dispatch_action(app, A_SW_START, at_ms(1000), {});
    REQUIRE_FALSE(app.sw.is_running());
    REQUIRE(app.sw_lap_file == lap_file);
}

TEST_CASE("A_SW_START reuses existing lap file on restart", "[actions]") {
    App app;
    dispatch_action(app, A_SW_START, t0(), {});
    dispatch_action(app, A_SW_START, at_ms(500), {}); // stop
    auto lap_file = app.sw_lap_file;
    dispatch_action(app, A_SW_START, at_ms(1000), {}); // restart
    REQUIRE(app.sw_lap_file == lap_file);
}

TEST_CASE("A_SW_LAP when not running does nothing", "[actions]") {
    App app;
    dispatch_action(app, A_SW_LAP, t0(), {});
    REQUIRE(app.sw.laps().empty());
}

TEST_CASE("A_SW_LAP when running records lap", "[actions]") {
    App app;
    dispatch_action(app, A_SW_START, t0(), {});
    dispatch_action(app, A_SW_LAP, at_ms(1000), {});
    REQUIRE(app.sw.laps().size() == 1);
    dispatch_action(app, A_SW_LAP, at_ms(2000), {});
    REQUIRE(app.sw.laps().size() == 2);
}

TEST_CASE("A_SW_RESET clears stopwatch state and lap file", "[actions]") {
    App app;
    dispatch_action(app, A_SW_START, t0(), {});
    dispatch_action(app, A_SW_RESET, t0(), {});
    REQUIRE_FALSE(app.sw.is_running());
    REQUIRE(app.sw_lap_file.empty());
}

TEST_CASE("A_SW_COPY sets copy_laps when laps exist", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_SW_COPY, t0(), {});
    REQUIRE_FALSE(r.copy_laps);

    dispatch_action(app, A_SW_START, t0(), {});
    dispatch_action(app, A_SW_LAP, at_ms(1000), {});
    r = dispatch_action(app, A_SW_COPY, at_ms(1500), {});
    REQUIRE(r.copy_laps);
}

TEST_CASE("A_SW_COPY works when stopwatch is paused with laps", "[actions]") {
    App app;
    dispatch_action(app, A_SW_START, t0(), {});
    dispatch_action(app, A_SW_LAP, at_ms(1000), {});
    dispatch_action(app, A_SW_START, at_ms(2000), {}); // stop
    REQUIRE_FALSE(app.sw.is_running());
    auto r = dispatch_action(app, A_SW_COPY, at_ms(2500), {});
    REQUIRE(r.copy_laps);
}

TEST_CASE("A_SW_GET opens file only when lap file exists on disk", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_SW_GET, t0(), {});
    REQUIRE_FALSE(r.open_file);

    auto tmp = std::filesystem::temp_directory_path() / "chronos-test-laps.txt";
    { std::ofstream f(tmp); f << "lap data"; }
    app.sw_lap_file = tmp;
    r = dispatch_action(app, A_SW_GET, t0(), {});
    REQUIRE(r.open_file);

    std::filesystem::remove(tmp);
}

TEST_CASE("A_SW_GET clears stale path when file is missing", "[actions]") {
    App app;
    app.sw_lap_file = std::filesystem::temp_directory_path() / "chronos-nonexistent.txt";
    auto r = dispatch_action(app, A_SW_GET, t0(), {});
    REQUIRE_FALSE(r.open_file);
    REQUIRE(app.sw_lap_file.empty());
}
