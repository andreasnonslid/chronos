#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include "../src/actions.hpp"
#include "../src/app.hpp"
#include "../src/config.hpp"

using namespace std::chrono;
using sc = steady_clock;

static sc::time_point t0() { return sc::time_point{}; }
static sc::time_point at_ms(int ms) { return t0() + milliseconds(ms); }

static void set_timer_dur(App& app, int idx, seconds dur) {
    app.timers[idx].dur = dur;
    app.timers[idx].t.reset();
    app.timers[idx].t.set(dur);
}

// ─── timer start / pause / resume ────────────────────────────────────────────

TEST_CASE("A_TMR_START starts untouched timer", "[actions]") {
    App app;
    REQUIRE_FALSE(app.timers[0].t.touched());
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    REQUIRE(app.timers[0].t.is_running());
    REQUIRE(app.timers[0].t.touched());
}

TEST_CASE("A_TMR_START pauses running timer", "[actions]") {
    App app;
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    dispatch_action(app, tmr_act(0, A_TMR_START), at_ms(1000), {});
    REQUIRE_FALSE(app.timers[0].t.is_running());
    REQUIRE(app.timers[0].t.touched());
}

TEST_CASE("A_TMR_START resumes paused timer", "[actions]") {
    App app;
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    dispatch_action(app, tmr_act(0, A_TMR_START), at_ms(1000), {}); // pause
    dispatch_action(app, tmr_act(0, A_TMR_START), at_ms(2000), {}); // resume
    REQUIRE(app.timers[0].t.is_running());
}

// ─── timer reset ─────────────────────────────────────────────────────────────

TEST_CASE("A_TMR_RST resets timer and clears notified flag", "[actions]") {
    App app;
    app.timers[0].notified = true;
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    dispatch_action(app, tmr_act(0, A_TMR_RST), at_ms(1000), {});
    REQUIRE_FALSE(app.timers[0].t.is_running());
    REQUIRE_FALSE(app.timers[0].t.touched());
    REQUIRE_FALSE(app.timers[0].notified);
}

// ─── timer add / delete ──────────────────────────────────────────────────────

TEST_CASE("A_TMR_ADD inserts timer after current index", "[actions]") {
    App app;
    REQUIRE(app.timers.size() == 1);
    auto r = dispatch_action(app, tmr_act(0, A_TMR_ADD), t0(), {});
    REQUIRE(app.timers.size() == 2);
    REQUIRE(r.resize);
    REQUIRE(r.save_config);
}

TEST_CASE("A_TMR_ADD respects MAX_TIMERS", "[actions]") {
    App app;
    while ((int)app.timers.size() < Config::MAX_TIMERS)
        dispatch_action(app, tmr_act(0, A_TMR_ADD), t0(), {});
    REQUIRE((int)app.timers.size() == Config::MAX_TIMERS);
    auto r = dispatch_action(app, tmr_act(0, A_TMR_ADD), t0(), {});
    REQUIRE((int)app.timers.size() == Config::MAX_TIMERS);
    REQUIRE_FALSE(r.resize);
}

TEST_CASE("A_TMR_DEL removes timer at index", "[actions]") {
    App app;
    dispatch_action(app, tmr_act(0, A_TMR_ADD), t0(), {}); // 2 timers
    auto r = dispatch_action(app, tmr_act(0, A_TMR_DEL), t0(), {});
    REQUIRE(app.timers.size() == 1);
    REQUIRE(r.resize);
    REQUIRE(r.save_config);
}

TEST_CASE("A_TMR_DEL does not remove last timer", "[actions]") {
    App app;
    REQUIRE(app.timers.size() == 1);
    auto r = dispatch_action(app, tmr_act(0, A_TMR_DEL), t0(), {});
    REQUIRE(app.timers.size() == 1);
    REQUIRE_FALSE(r.resize);
}

// ─── reset all timers ───────────────────────────────────────────────────────

TEST_CASE("A_TMR_RST_ALL resets all timers", "[actions]") {
    App app;
    dispatch_action(app, tmr_act(0, A_TMR_ADD), t0(), {});
    set_timer_dur(app, 0, seconds{300});
    set_timer_dur(app, 1, seconds{600});
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    dispatch_action(app, tmr_act(1, A_TMR_START), t0(), {});
    REQUIRE(app.timers[0].t.is_running());
    REQUIRE(app.timers[1].t.is_running());
    auto r = dispatch_action(app, A_TMR_RST_ALL, at_ms(1000), {});
    REQUIRE_FALSE(app.timers[0].t.is_running());
    REQUIRE_FALSE(app.timers[1].t.is_running());
    REQUIRE_FALSE(app.timers[0].t.touched());
    REQUIRE_FALSE(app.timers[1].t.touched());
    REQUIRE(r.save_config);
}

TEST_CASE("A_TMR_RST_ALL clears notified flags", "[actions]") {
    App app;
    app.timers[0].notified = true;
    auto r = dispatch_action(app, A_TMR_RST_ALL, t0(), {});
    REQUIRE_FALSE(app.timers[0].notified);
    REQUIRE(r.save_config);
}

TEST_CASE("A_TMR_RST_ALL resets Pomodoro state on all timers", "[actions]") {
    App app;
    dispatch_action(app, tmr_act(0, A_TMR_ADD), t0(), {});
    app.timers[0].pomodoro = true;
    app.timers[0].pomodoro_phase = PomodoroPhase::ShortBreak1;
    app.timers[0].pomodoro_work_elapsed = seconds{500};
    auto r = dispatch_action(app, A_TMR_RST_ALL, t0(), {});
    REQUIRE(app.timers[0].pomodoro_phase == PomodoroPhase::Work1);
    REQUIRE(app.timers[0].pomodoro_work_elapsed == seconds{0});
    REQUIRE(r.save_config);
}

// ─── zero-duration timer guard ───────────────────────────────────────────────

TEST_CASE("A_TMR_START does not start untouched zero-duration timer", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{0});
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    REQUIRE_FALSE(app.timers[0].t.touched());
    REQUIRE_FALSE(app.timers[0].t.is_running());
}
