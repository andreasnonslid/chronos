#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include "actions.hpp"
#include "app.hpp"
#include "test_helpers.hpp"

using namespace std::chrono;
using sc = steady_clock;

using test_helpers::at_ms;
using test_helpers::t0;

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

// Build a temp path unique to this run so leftover artifacts from a previous
// crashed run cannot make these tests flake. Catch2 runs the test binary
// sequentially within one process, so an atomic counter combined with the
// monotonic clock at process start is enough; no PID dance needed.
static std::filesystem::path unique_tmp(std::string_view suffix) {
    static std::atomic<unsigned long long> seq{0};
    static const auto start_ns =
        std::chrono::steady_clock::now().time_since_epoch().count();
    auto n = seq.fetch_add(1, std::memory_order_relaxed);
    return std::filesystem::temp_directory_path()
           / (std::string{"chronos-test-"} + std::to_string(start_ns) + "-"
              + std::to_string(n) + "-" + std::string{suffix});
}

TEST_CASE("actions-stopwatch: given unwritable lap-file path when A_SW_LAP dispatched"
          " then lap_write_failed is set",
          "[actions][actions-stopwatch]") {
    App app;
    dispatch_action(app, A_SW_START, t0(), {});
    // Point the lap file inside a unique directory we never create so the
    // ofstream open is guaranteed to fail; the dispatch path must record it.
    auto bad_dir = unique_tmp("no-such-dir");
    std::filesystem::remove_all(bad_dir); // belt-and-braces
    app.sw_lap_file = bad_dir / "laps.txt";
    REQUIRE_FALSE(std::filesystem::exists(bad_dir));
    REQUIRE_FALSE(app.lap_write_failed);
    dispatch_action(app, A_SW_LAP, at_ms(1000), {});
    REQUIRE(app.lap_write_failed);
}

TEST_CASE("actions-stopwatch: given writable lap-file path when A_SW_LAP dispatched"
          " then lap_write_failed stays false",
          "[actions][actions-stopwatch]") {
    auto tmp = unique_tmp("good-laps.txt");
    std::filesystem::remove(tmp);
    App app;
    dispatch_action(app, A_SW_START, t0(), {});
    app.sw_lap_file = tmp;
    dispatch_action(app, A_SW_LAP, at_ms(1000), {});
    REQUIRE_FALSE(app.lap_write_failed);
    std::filesystem::remove(tmp);
}
