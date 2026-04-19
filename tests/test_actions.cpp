#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include "../src/actions.hpp"
#include "../src/app.hpp"
#include "../src/config.hpp"

using namespace std::chrono;
using sc = steady_clock;

static sc::time_point t0() { return sc::time_point{}; }
static sc::time_point at_ms(int ms) { return t0() + milliseconds(ms); }

// Reset timer[idx] duration without touching it
static void set_timer_dur(App& app, int idx, seconds dur) {
    app.timers[idx].dur = dur;
    app.timers[idx].t.reset();
    app.timers[idx].t.set(dur);
}

// ─── tmr_act ─────────────────────────────────────────────────────────────────

TEST_CASE("tmr_act encodes timer index and offset", "[actions]") {
    REQUIRE(tmr_act(0, A_TMR_HUP) == A_TMR_BASE + 0);
    REQUIRE(tmr_act(0, A_TMR_START) == A_TMR_BASE + A_TMR_START);
    REQUIRE(tmr_act(1, A_TMR_HUP) == A_TMR_BASE + TMR_STRIDE);
    REQUIRE(tmr_act(2, A_TMR_DEL) == A_TMR_BASE + 2 * TMR_STRIDE + A_TMR_DEL);
}

// ─── wants_blink ─────────────────────────────────────────────────────────────

TEST_CASE("wants_blink returns false for toggle and start actions", "[actions]") {
    REQUIRE_FALSE(wants_blink(A_TOPMOST));
    REQUIRE_FALSE(wants_blink(A_SHOW_CLK));
    REQUIRE_FALSE(wants_blink(A_SHOW_SW));
    REQUIRE_FALSE(wants_blink(A_SHOW_TMR));
    REQUIRE_FALSE(wants_blink(A_SW_START));
    REQUIRE_FALSE(wants_blink(A_SW_COPY));
    REQUIRE_FALSE(wants_blink(tmr_act(0, A_TMR_START)));
    REQUIRE_FALSE(wants_blink(tmr_act(0, A_TMR_ADD)));
    REQUIRE_FALSE(wants_blink(tmr_act(0, A_TMR_DEL)));
}

TEST_CASE("wants_blink returns true for adjust, lap, and reset actions", "[actions]") {
    REQUIRE(wants_blink(A_SW_LAP));
    REQUIRE(wants_blink(A_SW_RESET));
    REQUIRE(wants_blink(tmr_act(0, A_TMR_HUP)));
    REQUIRE(wants_blink(tmr_act(0, A_TMR_HDN)));
    REQUIRE(wants_blink(tmr_act(0, A_TMR_MUP)));
    REQUIRE(wants_blink(tmr_act(0, A_TMR_MDN)));
    REQUIRE(wants_blink(tmr_act(0, A_TMR_SUP)));
    REQUIRE(wants_blink(tmr_act(0, A_TMR_SDN)));
    REQUIRE(wants_blink(tmr_act(0, A_TMR_RST)));
}

// ─── visibility toggles ──────────────────────────────────────────────────────

TEST_CASE("A_TOPMOST toggles topmost flag", "[actions]") {
    App app;
    REQUIRE_FALSE(app.topmost);
    auto r = dispatch_action(app, A_TOPMOST, t0(), {});
    REQUIRE(app.topmost);
    REQUIRE(r.set_topmost);
    REQUIRE(r.save_config);
    REQUIRE_FALSE(r.resize);

    dispatch_action(app, A_TOPMOST, t0(), {});
    REQUIRE_FALSE(app.topmost);
}

TEST_CASE("A_SHOW_CLK toggles clock visibility", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_SHOW_CLK, t0(), {});
    REQUIRE_FALSE(app.show_clk);
    REQUIRE(r.resize);
    REQUIRE(r.save_config);
    REQUIRE_FALSE(r.set_topmost);

    dispatch_action(app, A_SHOW_CLK, t0(), {});
    REQUIRE(app.show_clk);
}

TEST_CASE("A_SHOW_SW toggles stopwatch visibility", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_SHOW_SW, t0(), {});
    REQUIRE_FALSE(app.show_sw);
    REQUIRE(r.resize);
    REQUIRE(r.save_config);
}

TEST_CASE("A_SHOW_TMR toggles timer visibility", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_SHOW_TMR, t0(), {});
    REQUIRE_FALSE(app.show_tmr);
    REQUIRE(r.resize);
    REQUIRE(r.save_config);
}

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

TEST_CASE("A_SW_GET opens file only when lap file is set", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_SW_GET, t0(), {});
    REQUIRE_FALSE(r.open_file);

    dispatch_action(app, A_SW_START, t0(), {});
    r = dispatch_action(app, A_SW_GET, t0(), {});
    REQUIRE(r.open_file);
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
    dispatch_action(app, tmr_act(0, A_TMR_ADD), t0(), {});
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

// ─── zero-duration timer guard ───────────────────────────────────────────────

TEST_CASE("A_TMR_START does not start untouched zero-duration timer", "[actions]") {
    App app;
    set_timer_dur(app, 0, seconds{0});
    dispatch_action(app, tmr_act(0, A_TMR_START), t0(), {});
    REQUIRE_FALSE(app.timers[0].t.touched());
    REQUIRE_FALSE(app.timers[0].t.is_running());
}

// ─── theme cycling ──────────────────────────────────────────────────────────

TEST_CASE("A_THEME cycles theme mode and signals apply_theme", "[actions]") {
    App app;
    REQUIRE(app.theme_mode == ThemeMode::Auto);

    auto r1 = dispatch_action(app, A_THEME, t0(), {});
    REQUIRE(app.theme_mode == ThemeMode::Dark);
    REQUIRE(r1.apply_theme);
    REQUIRE(r1.save_config);

    auto r2 = dispatch_action(app, A_THEME, t0(), {});
    REQUIRE(app.theme_mode == ThemeMode::Light);
    REQUIRE(r2.apply_theme);
    REQUIRE(r2.save_config);

    auto r3 = dispatch_action(app, A_THEME, t0(), {});
    REQUIRE(app.theme_mode == ThemeMode::Auto);
    REQUIRE(r3.apply_theme);
    REQUIRE(r3.save_config);
}

TEST_CASE("A_THEME does not set resize or set_topmost", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_THEME, t0(), {});
    REQUIRE_FALSE(r.resize);
    REQUIRE_FALSE(r.set_topmost);
    REQUIRE_FALSE(r.open_file);
    REQUIRE_FALSE(r.copy_laps);
}

// ─── out-of-range index ──────────────────────────────────────────────────────

TEST_CASE("Out-of-range timer index does not crash or change state", "[actions]") {
    App app;
    auto dur_before = app.timers[0].dur;
    dispatch_action(app, tmr_act(5, A_TMR_HUP), t0(), {});
    REQUIRE(app.timers.size() == 1);
    REQUIRE(app.timers[0].dur == dur_before);
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
