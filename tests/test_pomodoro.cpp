#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <sstream>
#include "../src/actions.hpp"
#include "../src/config_serial.hpp"
#include "../src/pomodoro.hpp"

using namespace std::chrono;
using sc = steady_clock;

static sc::time_point t0() { return sc::time_point{}; }

// ─── pomodoro_phase_secs ──────────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_secs: work phases are 25 minutes", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::Work1) == POMODORO_WORK_SECS);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::Work2) == POMODORO_WORK_SECS);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::Work3) == POMODORO_WORK_SECS);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::Work4) == POMODORO_WORK_SECS);
}

TEST_CASE("pomodoro_phase_secs: short break phases are 5 minutes", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::ShortBreak1) == POMODORO_SHORT_BREAK_SECS);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::ShortBreak2) == POMODORO_SHORT_BREAK_SECS);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::ShortBreak3) == POMODORO_SHORT_BREAK_SECS);
}

TEST_CASE("pomodoro_phase_secs: phase LongBreak is a long break", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::LongBreak) == POMODORO_LONG_BREAK_SECS);
}

TEST_CASE("pomodoro_phase_secs: all valid phases produce expected values", "[pomodoro]") {
    for (int p = 0; p < POMODORO_PHASE_COUNT; ++p)
        REQUIRE_NOTHROW(pomodoro_phase_secs(static_cast<PomodoroPhase>(p)));
}

// ─── pomodoro_phase_label ─────────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_label: work phase labels", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(PomodoroPhase::Work1) == L"Work 1/4");
    REQUIRE(pomodoro_phase_label(PomodoroPhase::Work2) == L"Work 2/4");
    REQUIRE(pomodoro_phase_label(PomodoroPhase::Work3) == L"Work 3/4");
    REQUIRE(pomodoro_phase_label(PomodoroPhase::Work4) == L"Work 4/4");
}

TEST_CASE("pomodoro_phase_label: short break phases", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(PomodoroPhase::ShortBreak1) == L"Short Break");
    REQUIRE(pomodoro_phase_label(PomodoroPhase::ShortBreak2) == L"Short Break");
    REQUIRE(pomodoro_phase_label(PomodoroPhase::ShortBreak3) == L"Short Break");
}

TEST_CASE("pomodoro_phase_label: LongBreak is Long Break", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(PomodoroPhase::LongBreak) == L"Long Break");
}

TEST_CASE("pomodoro_phase_label: all valid phases produce expected labels", "[pomodoro]") {
    for (int p = 0; p < POMODORO_PHASE_COUNT; ++p)
        REQUIRE_FALSE(pomodoro_phase_label(static_cast<PomodoroPhase>(p)).empty());
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
    ts.pomodoro_phase = PomodoroPhase::ShortBreak3;
    ts.dur = seconds{POMODORO_SHORT_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    dispatch_action(app, A_TMR_BASE + A_TMR_RST, t0(), {});

    REQUIRE(ts.pomodoro_phase == PomodoroPhase::Work1);
    REQUIRE(ts.dur == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.label == pomodoro_phase_label(PomodoroPhase::Work1));
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


// ─── Custom Pomodoro durations ────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_secs: custom durations used when provided", "[pomodoro]") {
    constexpr int WORK = 50 * 60;
    constexpr int SHORT = 10 * 60;
    constexpr int LONG = 20 * 60;
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::Work1,       WORK, SHORT, LONG) == WORK);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::ShortBreak1, WORK, SHORT, LONG) == SHORT);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::ShortBreak2, WORK, SHORT, LONG) == SHORT);
    REQUIRE(pomodoro_phase_secs(PomodoroPhase::LongBreak,   WORK, SHORT, LONG) == LONG);
}

TEST_CASE("pomodoro_phase_secs: default args match constants", "[pomodoro]") {
    for (int p = 0; p < POMODORO_PHASE_COUNT; ++p) {
        auto phase = static_cast<PomodoroPhase>(p);
        REQUIRE(pomodoro_phase_secs(phase) == pomodoro_phase_secs(phase, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS));
    }
}

TEST_CASE("Config pomodoro durations default values", "[pomodoro][config]") {
    Config c;
    REQUIRE(c.pomodoro_work_secs == 25 * 60);
    REQUIRE(c.pomodoro_short_secs == 5 * 60);
    REQUIRE(c.pomodoro_long_secs == 15 * 60);
}

TEST_CASE("Config pomodoro durations round-trip", "[pomodoro][config]") {
    Config orig;
    orig.pomodoro_work_secs = 50 * 60;
    orig.pomodoro_short_secs = 10 * 60;
    orig.pomodoro_long_secs = 20 * 60;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.pomodoro_work_secs == 50 * 60);
    REQUIRE(back.pomodoro_short_secs == 10 * 60);
    REQUIRE(back.pomodoro_long_secs == 20 * 60);
}

TEST_CASE("Config pomodoro durations not written at defaults", "[pomodoro][config]") {
    Config orig;
    // durations at default values — should produce zero config change
    REQUIRE(orig.pomodoro_work_secs == 25 * 60);

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("pomodoro_work") == std::string::npos);
}

TEST_CASE("Config pomodoro work elapsed round-trip", "[pomodoro][config]") {
    Config orig;
    orig.num_timers = 1;
    orig.timer_pomodoro[0] = true;
    orig.timer_pomodoro_phase[0] = 3;
    orig.timer_pomodoro_work_secs[0] = 75 * 60;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.timer_pomodoro_work_secs[0] == 75 * 60);
}

TEST_CASE("Config pomodoro work elapsed not written when zero", "[pomodoro][config]") {
    Config orig;
    orig.num_timers = 1;
    orig.timer_pomodoro[0] = true;
    orig.timer_pomodoro_work_secs[0] = 0;

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("pomodoro_work_secs") == std::string::npos);
}

TEST_CASE("Config pomodoro work elapsed not written when pomodoro inactive", "[pomodoro][config]") {
    Config orig;
    orig.num_timers = 1;
    orig.timer_pomodoro[0] = false;
    orig.timer_pomodoro_work_secs[0] = 5000;

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("pomodoro_work_secs") == std::string::npos);
}

TEST_CASE("A_TMR_RST on Pomodoro timer resets work elapsed", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = true;
    ts.pomodoro_phase = PomodoroPhase::ShortBreak2;
    ts.pomodoro_work_elapsed = seconds{75 * 60};
    ts.dur = seconds{POMODORO_SHORT_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    dispatch_action(app, A_TMR_BASE + A_TMR_RST, t0(), {});

    REQUIRE(ts.pomodoro_work_elapsed == seconds{0});
}

TEST_CASE("Config pomodoro durations written when any differs from default", "[pomodoro][config]") {
    Config orig;
    orig.pomodoro_work_secs = 30 * 60; // only one changed

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("pomodoro_work=1800") != std::string::npos);
    REQUIRE(os.str().find("pomodoro_short=300") != std::string::npos);
    REQUIRE(os.str().find("pomodoro_long=900") != std::string::npos);
}
