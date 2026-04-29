#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include "../src/actions.hpp"
#include "../src/app.hpp"

using namespace std::chrono;
using sc = steady_clock;

static sc::time_point t0() { return sc::time_point{}; }

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
    REQUIRE_FALSE(wants_blink(A_CLK_CYCLE));
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

// ─── clock view cycling ─────────────────────────────────────────────────────

TEST_CASE("A_CLK_CYCLE cycles through all clock views", "[actions]") {
    App app;
    REQUIRE(app.clock_view == ClockView::H24_HMS);

    auto r1 = dispatch_action(app, A_CLK_CYCLE, t0(), {});
    REQUIRE(app.clock_view == ClockView::H24_HM);
    REQUIRE(r1.save_config);

    auto r2 = dispatch_action(app, A_CLK_CYCLE, t0(), {});
    REQUIRE(app.clock_view == ClockView::H12_HMS);
    REQUIRE(r2.save_config);

    auto r3 = dispatch_action(app, A_CLK_CYCLE, t0(), {});
    REQUIRE(app.clock_view == ClockView::H12_HM);
    REQUIRE(r3.save_config);

    auto r4 = dispatch_action(app, A_CLK_CYCLE, t0(), {});
    REQUIRE(app.clock_view == ClockView::H24_HMS);
    REQUIRE(r4.save_config);
}

TEST_CASE("A_CLK_CYCLE does not set resize or apply_theme", "[actions]") {
    App app;
    auto r = dispatch_action(app, A_CLK_CYCLE, t0(), {});
    REQUIRE_FALSE(r.resize);
    REQUIRE_FALSE(r.set_topmost);
    REQUIRE_FALSE(r.apply_theme);
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
