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

TEST_CASE("pomodoro_phase_secs: last phase is a long break", "[pomodoro]") {
    REQUIRE(pomodoro_phase_secs(7) == POMODORO_LONG_BREAK_SECS);
}

TEST_CASE("pomodoro_phase_secs: all valid phases produce expected values", "[pomodoro]") {
    for (int p = 0; p < pomodoro_phase_count(POMODORO_DEFAULT_CADENCE); ++p)
        REQUIRE_NOTHROW(pomodoro_phase_secs(p));
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

TEST_CASE("pomodoro_phase_label: last phase is Long Break", "[pomodoro]") {
    REQUIRE(pomodoro_phase_label(7) == L"Long Break");
}

TEST_CASE("pomodoro_phase_label: all valid phases produce expected labels", "[pomodoro]") {
    for (int p = 0; p < pomodoro_phase_count(POMODORO_DEFAULT_CADENCE); ++p)
        REQUIRE_FALSE(pomodoro_phase_label(p).empty());
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


// ─── Custom Pomodoro durations ────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_secs: custom durations used when provided", "[pomodoro]") {
    constexpr int WORK = 50 * 60;
    constexpr int SHORT = 10 * 60;
    constexpr int LONG = 20 * 60;
    REQUIRE(pomodoro_phase_secs(0, WORK, SHORT, LONG) == WORK);
    REQUIRE(pomodoro_phase_secs(1, WORK, SHORT, LONG) == SHORT);
    REQUIRE(pomodoro_phase_secs(3, WORK, SHORT, LONG) == SHORT);
    REQUIRE(pomodoro_phase_secs(7, WORK, SHORT, LONG) == LONG);
}

TEST_CASE("pomodoro_phase_secs: default args match constants", "[pomodoro]") {
    for (int p = 0; p < pomodoro_phase_count(POMODORO_DEFAULT_CADENCE); ++p) {
        REQUIRE(pomodoro_phase_secs(p) == pomodoro_phase_secs(p, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS));
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
    ts.pomodoro_phase = 3;
    ts.pomodoro_work_elapsed = seconds{75 * 60};
    ts.dur = seconds{POMODORO_SHORT_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    dispatch_action(app, A_TMR_BASE + A_TMR_RST, t0(), {});

    REQUIRE(ts.pomodoro_work_elapsed == seconds{0});
}

// ─── A_TMR_SKIP ──────────────────────────────────────────────────────────────

TEST_CASE("A_TMR_SKIP advances phase and starts next timer", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    auto r = dispatch_action(app, tmr_act(0, A_TMR_SKIP), t0() + seconds{600}, {});

    REQUIRE(ts.pomodoro_phase == 1);
    REQUIRE(ts.dur == seconds{POMODORO_SHORT_BREAK_SECS});
    REQUIRE(ts.label == pomodoro_phase_label(1));
    REQUIRE(ts.t.is_running());
    REQUIRE(r.save_config);
}

TEST_CASE("A_TMR_SKIP on work phase credits actual elapsed time", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.pomodoro_work_elapsed = seconds{0};
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    dispatch_action(app, tmr_act(0, A_TMR_SKIP), t0() + seconds{600}, {});

    REQUIRE(ts.pomodoro_work_elapsed == seconds{600});
}

TEST_CASE("A_TMR_SKIP on break phase does not credit work time", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = true;
    ts.pomodoro_phase = 1;
    ts.pomodoro_work_elapsed = seconds{25 * 60};
    ts.dur = seconds{POMODORO_SHORT_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    dispatch_action(app, tmr_act(0, A_TMR_SKIP), t0() + seconds{60}, {});

    REQUIRE(ts.pomodoro_work_elapsed == seconds{25 * 60});
    REQUIRE(ts.pomodoro_phase == 2);
}

TEST_CASE("A_TMR_SKIP is no-op on non-Pomodoro timer", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = false;
    ts.dur = seconds{300};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    auto r = dispatch_action(app, tmr_act(0, A_TMR_SKIP), t0() + seconds{60}, {});

    REQUIRE_FALSE(r.save_config);
    REQUIRE(ts.dur == seconds{300});
}

TEST_CASE("A_TMR_SKIP is no-op when timer not touched", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);

    auto r = dispatch_action(app, tmr_act(0, A_TMR_SKIP), t0(), {});

    REQUIRE_FALSE(r.save_config);
    REQUIRE(ts.pomodoro_phase == 0);
}

TEST_CASE("A_TMR_SKIP wraps from LongBreak back to Work1", "[pomodoro][actions]") {
    App app;
    auto& ts = app.timers[0];
    ts.pomodoro = true;
    ts.pomodoro_phase = 7;
    ts.dur = seconds{POMODORO_LONG_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    dispatch_action(app, tmr_act(0, A_TMR_SKIP), t0() + seconds{60}, {});

    REQUIRE(ts.pomodoro_phase == 0);
    REQUIRE(ts.dur == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.label == pomodoro_phase_label(0));
}

// ─── advance_pomodoro_phase ──────────────────────────────────────────────────

TEST_CASE("advance: work expires -> short break, work_elapsed increases", "[pomodoro][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.pomodoro_work_elapsed = seconds{0};
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS,
                           POMODORO_DEFAULT_CADENCE, true, t0() + seconds{POMODORO_WORK_SECS});

    REQUIRE(ts.pomodoro_phase == 1);
    REQUIRE(ts.dur == seconds{POMODORO_SHORT_BREAK_SECS});
    REQUIRE(ts.label == L"Short Break");
    REQUIRE(ts.pomodoro_work_elapsed == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.t.is_running());
    REQUIRE_FALSE(ts.notified);
}

TEST_CASE("advance: short break expires -> next work phase, work_elapsed unchanged", "[pomodoro][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 1;
    ts.pomodoro_work_elapsed = seconds{POMODORO_WORK_SECS};
    ts.dur = seconds{POMODORO_SHORT_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS,
                           POMODORO_DEFAULT_CADENCE, true, t0() + seconds{POMODORO_SHORT_BREAK_SECS});

    REQUIRE(ts.pomodoro_phase == 2);
    REQUIRE(ts.dur == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.label == L"Work 2/4");
    REQUIRE(ts.pomodoro_work_elapsed == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.t.is_running());
}

TEST_CASE("advance: Work4 expires -> long break", "[pomodoro][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 6;
    ts.pomodoro_work_elapsed = seconds{3 * POMODORO_WORK_SECS};
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS,
                           POMODORO_DEFAULT_CADENCE, true, t0() + seconds{POMODORO_WORK_SECS});

    REQUIRE(ts.pomodoro_phase == 7);
    REQUIRE(ts.dur == seconds{POMODORO_LONG_BREAK_SECS});
    REQUIRE(ts.label == L"Long Break");
    REQUIRE(ts.pomodoro_work_elapsed == seconds{4 * POMODORO_WORK_SECS});
}

TEST_CASE("advance: long break expires -> wraps to Work1, work_elapsed unchanged", "[pomodoro][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 7;
    ts.pomodoro_work_elapsed = seconds{4 * POMODORO_WORK_SECS};
    ts.dur = seconds{POMODORO_LONG_BREAK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS,
                           POMODORO_DEFAULT_CADENCE, true, t0() + seconds{POMODORO_LONG_BREAK_SECS});

    REQUIRE(ts.pomodoro_phase == 0);
    REQUIRE(ts.dur == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.label == L"Work 1/4");
    REQUIRE(ts.pomodoro_work_elapsed == seconds{4 * POMODORO_WORK_SECS});
    REQUIRE(ts.t.is_running());
}

TEST_CASE("advance: custom durations flow through correctly", "[pomodoro][advance]") {
    constexpr int WORK = 50 * 60;
    constexpr int SHORT = 10 * 60;
    constexpr int LONG = 30 * 60;

    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.dur = seconds{WORK};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, WORK, SHORT, LONG, POMODORO_DEFAULT_CADENCE, true, t0() + seconds{WORK});

    REQUIRE(ts.dur == seconds{SHORT});

    ts.pomodoro_phase = 6;
    ts.dur = seconds{WORK};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, WORK, SHORT, LONG, POMODORO_DEFAULT_CADENCE, true, t0() + seconds{WORK});

    REQUIRE(ts.dur == seconds{LONG});
    REQUIRE(ts.label == L"Long Break");
}

TEST_CASE("advance: notified flag is cleared", "[pomodoro][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.notified = true;
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS,
                           POMODORO_DEFAULT_CADENCE, true, t0() + seconds{POMODORO_WORK_SECS});

    REQUIRE_FALSE(ts.notified);
}

TEST_CASE("advance: full cycle through all 8 phases", "[pomodoro][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.pomodoro_work_elapsed = seconds{0};
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    auto now = t0();
    int count = pomodoro_phase_count(POMODORO_DEFAULT_CADENCE);
    for (int i = 0; i < count; ++i) {
        now += ts.dur;
        advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS, POMODORO_LONG_BREAK_SECS,
                               POMODORO_DEFAULT_CADENCE, true, now);
    }

    REQUIRE(ts.pomodoro_phase == 0);
    REQUIRE(ts.dur == seconds{POMODORO_WORK_SECS});
    REQUIRE(ts.label == L"Work 1/4");
    REQUIRE(ts.pomodoro_work_elapsed == seconds{4 * POMODORO_WORK_SECS});
    REQUIRE(ts.t.is_running());
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

// ─── Configurable cadence ────────────────────────────────────────────────────

TEST_CASE("pomodoro_phase_count reflects cadence", "[pomodoro][cadence]") {
    REQUIRE(pomodoro_phase_count(1) == 2);
    REQUIRE(pomodoro_phase_count(4) == 8);
    REQUIRE(pomodoro_phase_count(6) == 12);
    REQUIRE(pomodoro_phase_count(10) == 20);
}

TEST_CASE("cadence 2: two work sessions then long break", "[pomodoro][cadence]") {
    constexpr int CAD = 2;
    REQUIRE(pomodoro_phase_label(0, CAD) == L"Work 1/2");
    REQUIRE(pomodoro_phase_label(1, CAD) == L"Short Break");
    REQUIRE(pomodoro_phase_label(2, CAD) == L"Work 2/2");
    REQUIRE(pomodoro_phase_label(3, CAD) == L"Long Break");
}

TEST_CASE("cadence 1: single work session then long break", "[pomodoro][cadence]") {
    constexpr int CAD = 1;
    REQUIRE(pomodoro_phase_count(CAD) == 2);
    REQUIRE(pomodoro_phase_label(0, CAD) == L"Work 1/1");
    REQUIRE(pomodoro_phase_label(1, CAD) == L"Long Break");
    REQUIRE(pomodoro_is_long_break(1, CAD));
}

TEST_CASE("cadence 6: six work sessions then long break", "[pomodoro][cadence]") {
    constexpr int CAD = 6;
    REQUIRE(pomodoro_phase_count(CAD) == 12);
    REQUIRE(pomodoro_phase_label(10, CAD) == L"Work 6/6");
    REQUIRE(pomodoro_phase_label(11, CAD) == L"Long Break");
    REQUIRE(pomodoro_is_long_break(11, CAD));
    REQUIRE_FALSE(pomodoro_is_long_break(9, CAD));
}

TEST_CASE("advance with custom cadence cycles correctly", "[pomodoro][cadence][advance]") {
    constexpr int CAD = 2;
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.pomodoro_work_elapsed = seconds{0};
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    auto now = t0();
    int count = pomodoro_phase_count(CAD);
    for (int i = 0; i < count; ++i) {
        now += ts.dur;
        advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS,
                               POMODORO_LONG_BREAK_SECS, CAD, true, now);
    }

    REQUIRE(ts.pomodoro_phase == 0);
    REQUIRE(ts.label == L"Work 1/2");
    REQUIRE(ts.pomodoro_work_elapsed == seconds{2 * POMODORO_WORK_SECS});
}

TEST_CASE("Config cadence round-trip", "[pomodoro][cadence][config]") {
    Config orig;
    orig.pomodoro_cadence = 6;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.pomodoro_cadence == 6);
}

TEST_CASE("Config cadence not written at default", "[pomodoro][cadence][config]") {
    Config orig;
    REQUIRE(orig.pomodoro_cadence == POMODORO_DEFAULT_CADENCE);

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("pomodoro_cadence") == std::string::npos);
}

TEST_CASE("Config cadence clamped to valid range", "[pomodoro][cadence][config]") {
    std::istringstream is("pomodoro_cadence=99\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.pomodoro_cadence == POMODORO_MAX_CADENCE);

    std::istringstream is2("pomodoro_cadence=0\n");
    Config c2;
    config_read(c2, is2);
    REQUIRE(c2.pomodoro_cadence == POMODORO_MIN_CADENCE);
}

// ─── Auto-start toggle ──────────────────────────────────────────────────────

TEST_CASE("advance with auto_start=false does not start timer", "[pomodoro][auto_start][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS,
                           POMODORO_LONG_BREAK_SECS, POMODORO_DEFAULT_CADENCE, false,
                           t0() + seconds{POMODORO_WORK_SECS});

    REQUIRE(ts.pomodoro_phase == 1);
    REQUIRE(ts.dur == seconds{POMODORO_SHORT_BREAK_SECS});
    REQUIRE_FALSE(ts.t.is_running());
    REQUIRE_FALSE(ts.t.touched());
}

TEST_CASE("advance with auto_start=true starts timer", "[pomodoro][auto_start][advance]") {
    TimerSlot ts;
    ts.pomodoro = true;
    ts.pomodoro_phase = 0;
    ts.dur = seconds{POMODORO_WORK_SECS};
    ts.t.reset();
    ts.t.set(ts.dur);
    ts.t.start(t0());

    advance_pomodoro_phase(ts, POMODORO_WORK_SECS, POMODORO_SHORT_BREAK_SECS,
                           POMODORO_LONG_BREAK_SECS, POMODORO_DEFAULT_CADENCE, true,
                           t0() + seconds{POMODORO_WORK_SECS});

    REQUIRE(ts.pomodoro_phase == 1);
    REQUIRE(ts.t.is_running());
}

TEST_CASE("Config auto_start round-trip", "[pomodoro][auto_start][config]") {
    Config orig;
    orig.pomodoro_auto_start = false;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE_FALSE(back.pomodoro_auto_start);
}

TEST_CASE("Config auto_start not written at default", "[pomodoro][auto_start][config]") {
    Config orig;
    REQUIRE(orig.pomodoro_auto_start == true);

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("pomodoro_auto_start") == std::string::npos);
}

TEST_CASE("Config auto_start defaults", "[pomodoro][auto_start][config]") {
    Config c;
    REQUIRE(c.pomodoro_auto_start == true);
    REQUIRE(c.pomodoro_cadence == POMODORO_DEFAULT_CADENCE);
}
