#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include "../src/actions.hpp"
#include "../src/app.hpp"
#include "../src/config.hpp"

using namespace std::chrono;
using sc = steady_clock;

static sc::time_point t0() { return sc::time_point{}; }

static void set_timer_dur(App& app, int idx, seconds dur) {
    app.timers[idx].dur = dur;
    app.timers[idx].t.reset();
    app.timers[idx].t.set(dur);
}

// ─── timer hour adjustments ──────────────────────────────────────────────────

TEST_CASE("A_TMR_HUP increments hours", "[actions]") {
    App app; // default 0h 1m 0s = 60s
    dispatch_action(app, tmr_act(0, A_TMR_HUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{3660}); // 1h 1m 0s
}

TEST_CASE("A_TMR_HUP wraps 24 to 0", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{24 * 3600}); // 24h 0m 0s
    dispatch_action(app, tmr_act(0, A_TMR_HUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{0}); // 0h 0m 0s
}

TEST_CASE("A_TMR_HDN decrements hours", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{3660}); // 1h 1m 0s
    dispatch_action(app, tmr_act(0, A_TMR_HDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{60}); // 0h 1m 0s
}

TEST_CASE("A_TMR_HDN wraps 0 to 24 and clamps to TIMER_MAX_SECS", "[actions]") {
    App app; // default 0h 1m 0s; wrapping h to 24 gives 24h 1m 0s = 86460 > TIMER_MAX_SECS
    dispatch_action(app, tmr_act(0, A_TMR_HDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS}); // clamped to 86400
}

// ─── timer minute adjustments ────────────────────────────────────────────────

TEST_CASE("A_TMR_MUP increments minutes", "[actions]") {
    App app; // 0h 1m 0s
    dispatch_action(app, tmr_act(0, A_TMR_MUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{120}); // 0h 2m 0s
}

TEST_CASE("A_TMR_MUP wraps 59 to 0", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{59 * 60}); // 0h 59m 0s
    dispatch_action(app, tmr_act(0, A_TMR_MUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{0}); // 0h 0m 0s
}

TEST_CASE("A_TMR_MDN decrements minutes", "[actions]") {
    App app; // 0h 1m 0s
    dispatch_action(app, tmr_act(0, A_TMR_MDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{0}); // 0h 0m 0s
}

TEST_CASE("A_TMR_MDN wraps 0 to 59", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{0}); // 0h 0m 0s
    dispatch_action(app, tmr_act(0, A_TMR_MDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{59 * 60}); // 0h 59m 0s
}

// ─── timer second adjustments ────────────────────────────────────────────────

TEST_CASE("A_TMR_SUP increments seconds", "[actions]") {
    App app; // 0h 1m 0s
    dispatch_action(app, tmr_act(0, A_TMR_SUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{61}); // 0h 1m 1s
}

TEST_CASE("A_TMR_SUP wraps 59 to 0", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{59}); // 0h 0m 59s
    dispatch_action(app, tmr_act(0, A_TMR_SUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{0}); // 0h 0m 0s
}

TEST_CASE("A_TMR_SDN decrements seconds", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{61}); // 0h 1m 1s
    dispatch_action(app, tmr_act(0, A_TMR_SDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{60}); // 0h 1m 0s
}

TEST_CASE("A_TMR_SDN wraps 0 to 59", "[actions]") {
    App app; // 0h 1m 0s (sec=0)
    dispatch_action(app, tmr_act(0, A_TMR_SDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{119}); // 0h 1m 59s
}

// ─── adjustments ignored on touched timer ────────────────────────────────────

TEST_CASE("Timer adjustments ignored when timer is touched", "[actions]") {
    App app;                                                 // default 0h 1m 0s
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {}); // start → touched
    REQUIRE(app.timers[0].t.touched());
    auto dur_before = app.timers[0].dur;
    dispatch_action(app, tmr_act(0, A_TMR_HUP), t0(), {});
    REQUIRE(app.timers[0].dur == dur_before);
}

// ─── timer adjustment clamping ───────────────────────────────────────────────

TEST_CASE("Timer adjustment clamps to TIMER_MAX_SECS", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{23 * 3600 + 59 * 60 + 59}); // 23:59:59
    dispatch_action(app, tmr_act(0, A_TMR_HUP), t0(), {});    // h → 24, total would be 89999
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
}

TEST_CASE("A_TMR_MUP at 24h boundary stays at TIMER_MAX_SECS", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{Config::TIMER_MAX_SECS}); // 24h 0m 0s
    dispatch_action(app, tmr_act(0, A_TMR_MUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS}); // still 24h 0m 0s
}

TEST_CASE("A_TMR_SUP at 24h boundary stays at TIMER_MAX_SECS", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{Config::TIMER_MAX_SECS}); // 24h 0m 0s
    dispatch_action(app, tmr_act(0, A_TMR_SUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS}); // still 24h 0m 0s
}

// ─── timer duration adjustment edge cases ────────────────────────────────────

TEST_CASE("A_TMR_MDN at 24h boundary stays at TIMER_MAX_SECS", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{Config::TIMER_MAX_SECS});
    dispatch_action(app, tmr_act(0, A_TMR_MDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
}

TEST_CASE("A_TMR_SDN at 24h boundary stays at TIMER_MAX_SECS", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{Config::TIMER_MAX_SECS});
    dispatch_action(app, tmr_act(0, A_TMR_SDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
}

TEST_CASE("A_TMR_HDN wraps and clamps when both minutes and seconds are nonzero", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{30 * 60 + 30}); // 0:30:30
    dispatch_action(app, tmr_act(0, A_TMR_HDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
}

TEST_CASE("All adjustments at 24h boundary stay clamped", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{Config::TIMER_MAX_SECS});
    dispatch_action(app, tmr_act(0, A_TMR_MUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
    dispatch_action(app, tmr_act(0, A_TMR_SUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
    dispatch_action(app, tmr_act(0, A_TMR_MDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
    dispatch_action(app, tmr_act(0, A_TMR_SDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{Config::TIMER_MAX_SECS});
}

TEST_CASE("Zero duration reached via adjustment prevents start", "[actions]") {
    App app; // default 0:01:00
    dispatch_action(app, tmr_act(0, A_TMR_MDN), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{0});
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    REQUIRE_FALSE(app.timers[0].t.is_running());
    REQUIRE_FALSE(app.timers[0].t.touched());
}

TEST_CASE("A_TMR_MUP wraps without carrying to hours", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{59 * 60 + 30}); // 0:59:30
    dispatch_action(app, tmr_act(0, A_TMR_MUP), t0(), {});
    REQUIRE(app.timers[0].dur == seconds{30}); // 0:00:30
}
