#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <sstream>
#include "../src/actions.hpp"
#include "../src/config.hpp"
#include "../src/pomodoro.hpp"

using namespace std::chrono;
using sc = steady_clock;

static sc::time_point t0() { return sc::time_point{}; }

// ─── pomodoro_phase_secs ──────────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_secs: work phases are 25 minutes", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(0) == POMODORO_WORK_SECS);
    REQUIRE(pomodoro_phase_secs(2) == POMODORO_WORK_SECS);
    REQUIRE(pomodoro_phase_secs(4) == POMODORO_WORK_SECS);
    REQUIRE(pomodoro_phase_secs(6) == POMODORO_WORK_SECS);
}

TEST_CASE("pomodoro_phase_secs: short break phases are 5 minutes", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(1) == POMODORO_SHORT_BREAK_SECS);
    REQUIRE(pomodoro_phase_secs(3) == POMODORO_SHORT_BREAK_SECS);
    REQUIRE(pomodoro_phase_secs(5) == POMODORO_SHORT_BREAK_SECS);
}

TEST_CASE("pomodoro_phase_secs: phase 7 is a long break", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(7) == POMODORO_LONG_BREAK_SECS);
}

TEST_CASE("pomodoro_phase_secs: phase wraps at 8", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(8)  == pomodoro_phase_secs(0));
    REQUIRE(pomodoro_phase_secs(9)  == pomodoro_phase_secs(1));
    REQUIRE(pomodoro_phase_secs(15) == pomodoro_phase_secs(7));
}

// ─── pomodoro_phase_label ─────────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_label: work phase labels", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(0) == L"Work 1/4");
    REQUIRE(pomodoro_phase_label(2) == L"Work 2/4");
    REQUIRE(pomodoro_phase_label(4) == L"Work 3/4");
    REQUIRE(pomodoro_phase_label(6) == L"Work 4/4");
}

TEST_CASE("pomodoro_phase_label: short break phases", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(1) == L"Short Break");
    REQUIRE(pomodoro_phase_label(3) == L"Short Break");
    REQUIRE(pomodoro_phase_label(5) == L"Short Break");
}

TEST_CASE("pomodoro_phase_label: phase 7 is Long Break", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(7) == L"Long Break");
}

TEST_CASE("pomodoro_phase_label: phase wraps at 8", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(8)  == pomodoro_phase_label(0));
    REQUIRE(pomodoro_phase_label(9)  == pomodoro_phase_label(1));
    REQUIRE(pomodoro_phase_label(15) == pomodoro_phase_label(7));
}

// ─── Config Pomodoro round-trip ───────────────────────────────────────────────

TEST_CASE("Config Pomodoro round-trip", "[pomodoro][config]") {
    Config orig;
    orig.num_timers = 2;
    orig.timer_pomodoro[0] = true;
    orig.timer_pomodoro_phase[0] = 3;
    orig.timer_pomodoro[1] = false;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.timer_pomodoro[0]);
    REQUIRE(back.timer_pomodoro_phase[0] == 3);
    REQUIRE_FALSE(back.timer_pomodoro[1]);
}

TEST_CASE("Config Pomodoro fields not written when inactive", "[pomodoro][config]") {
    Config orig;
    orig.num_timers = 1;
    orig.timer_pomodoro[0] = false;
    orig.timer_pomodoro_phase[0] = 5;

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("pomodoro") == std::string::npos);
}

// ─── A_TMR_RST on a Pomodoro timer ───────────────────────────────────────────

TEST_CASE("A_TMR_RST on Pomodoro timer resets phase to 0", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = true;
    ts.pomodoro_phase = 5;
    ts.dur = seconds{POMODORO_SHORT_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    dispatch_action(app, A_TMR_BASE + A_TMR_RST, t0(), {});

    REQUIRE(ts.pomodoro_phase == 0);
    REQUIRE(ts.dur == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.label == pomodoro_phase_label(0));
    REQUIRE_FALSE(ts.t.is_running());
}

TEST_CASE("A_TMR_RST on non-Pomodoro timer leaves label unchanged", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = false;
    ts.label = L"Tea";
    ts.dur = seconds{300};
    ts.t.reset();
    ts.t.set(ts.dur);

    dispatch_action(app, A_TMR_BASE + A_TMR_RST, t0(), {});

    REQUIRE_FALSE(ts.pomodoro);
    REQUIRE(ts.label == L"Tea");
}

// ─── Negative phase values ────────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_secs: negative multiples of 8 behave like phase 0", "[pomodoro]") {
    // -8 % 8 == 0 in C++, so these map to work phases
    REQUIRE(pomodoro_phase_secs(-8)  == POMODORO_WORK_SECS);
    REQUIRE(pomodoro_phase_secs(-16) == POMODORO_WORK_SECS);
}

TEST_CASE("pomodoro_phase_secs: negative phases do not crash", "[pomodoro]") {
    // Verify no crash or UB; C++ truncates % toward zero
    REQUIRE_NOTHROW(pomodoro_phase_secs(-1));
    REQUIRE_NOTHROW(pomodoro_phase_secs(-7));
}

TEST_CASE("pomodoro_phase_label: negative multiples of 8 behave like phase 0", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(-8)  == pomodoro_phase_label(0));
    REQUIRE(pomodoro_phase_label(-16) == pomodoro_phase_label(0));
}

TEST_CASE("pomodoro_phase_label: negative phases do not crash", "[pomodoro]") {
    REQUIRE_NOTHROW(pomodoro_phase_label(-1));
    REQUIRE_NOTHROW(pomodoro_phase_label(-7));
}
