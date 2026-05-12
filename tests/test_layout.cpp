#include <catch2/catch_test_macros.hpp>
#include "../src/layout.hpp"

// ── dpi_scale ───────────────────────────────────────────────────────────────

TEST_CASE("Layout dpi_scale: 96 DPI is identity", "[layout]") {
    Layout l;
    l.dpi = 96;
    REQUIRE(l.dpi_scale(100) == 100);
    REQUIRE(l.dpi_scale(0) == 0);
}

TEST_CASE("Layout dpi_scale: 144 DPI (1.5x)", "[layout]") {
    Layout l;
    l.dpi = 144;
    REQUIRE(l.dpi_scale(100) == 150);
    REQUIRE(l.dpi_scale(10) == 15);
}

TEST_CASE("Layout dpi_scale: 192 DPI (2x)", "[layout]") {
    Layout l;
    l.dpi = 192;
    REQUIRE(l.dpi_scale(100) == 200);
    REQUIRE(l.dpi_scale(36) == 72);
}

// ── update_for_dpi ──────────────────────────────────────────────────────────

TEST_CASE("Layout update_for_dpi: standard 96 DPI", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    REQUIRE(l.bar_h == 36);
    REQUIRE(l.clk_h == 62);
    REQUIRE(l.sw_h == 116);
    REQUIRE(l.tmr_h == 114);
    REQUIRE(l.btn_h == 28);
}

TEST_CASE("Layout update_for_dpi: 192 DPI doubles values", "[layout]") {
    Layout l;
    l.update_for_dpi(192);
    REQUIRE(l.bar_h == 72);
    REQUIRE(l.clk_h == 124);
    REQUIRE(l.sw_h == 232);
    REQUIRE(l.tmr_h == 228);
    REQUIRE(l.btn_h == 56);
}

// ── bar_min_client_w ────────────────────────────────────────────────────────

TEST_CASE("Layout bar_min_client_w: correct at 96 DPI", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    // w_pin(44) + w_clk(48) + w_sw(76) + w_tmr(54) + w_set(32) + 4*bar_gap(6) + 2*dpi_scale(8)
    int expected = 44 + 48 + 76 + 54 + 32 + 4 * 6 + 2 * 8;
    REQUIRE(l.bar_min_client_w() == expected);
}

// ── client_height_for ───────────────────────────────────────────────────────

TEST_CASE("client_height_for: all sections visible", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{true, true, true, 2};
    // bar_h + clk_h + sw_h + 2*tmr_h
    REQUIRE(client_height_for(l, s) == 36 + 62 + 116 + 2 * 114);
}

TEST_CASE("client_height_for: none visible", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{false, false, false, 1};
    REQUIRE(client_height_for(l, s) == 36); // just bar
}

TEST_CASE("client_height_for: only clock", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{true, false, false, 1};
    REQUIRE(client_height_for(l, s) == 36 + 62);
}

TEST_CASE("client_height_for: timers with 3 slots", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{false, false, true, 3};
    REQUIRE(client_height_for(l, s) == 36 + 3 * 114);
}

// ── timer_index_at_y ───────────────────────────────────────────────────────

TEST_CASE("timer_index_at_y: click in timer region", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{true, true, true, 2};
    int top = l.bar_h + l.clk_h + l.sw_h;
    REQUIRE(timer_index_at_y(l, s, top + 1) == 0);
    REQUIRE(timer_index_at_y(l, s, top + l.tmr_h + 1) == 1);
}

TEST_CASE("timer_index_at_y: click above timer region", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{true, true, true, 1};
    REQUIRE(timer_index_at_y(l, s, l.bar_h) == -1);
}

TEST_CASE("timer_index_at_y: timers hidden", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{false, false, false, 0};
    REQUIRE(timer_index_at_y(l, s, 50) == -1);
}

TEST_CASE("timer_index_at_y: click below all timers", "[layout]") {
    Layout l;
    l.update_for_dpi(96);
    LayoutState s{false, false, true, 1};
    int bottom = l.bar_h + l.tmr_h;
    REQUIRE(timer_index_at_y(l, s, bottom + 10) == -1);
}
