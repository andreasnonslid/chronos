#include <catch2/catch_test_macros.hpp>
#include "toolbar_strip.hpp"

// ── toolbar_strip_btn_update (render_titlebar site) ──────────────────────────
// Handles click-toggle and hover-open. Never closes the strip.

TEST_CASE("btn_update: button hover opens closed strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_btn_update(false, false, true) == true);
}

TEST_CASE("btn_update: button hover keeps open strip open", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_btn_update(true, false, true) == true);
}

TEST_CASE("btn_update: no hover leaves closed strip closed", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_btn_update(false, false, false) == false);
}

TEST_CASE("btn_update: no hover does not close open strip (closing is strip's job)", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_btn_update(true, false, false) == true);
}

TEST_CASE("btn_update: click opens closed strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_btn_update(false, true, false) == true);
}

TEST_CASE("btn_update: click closes open strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_btn_update(true, true, false) == false);
}

TEST_CASE("btn_update: click takes priority over hover when strip is open", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_btn_update(true, true, true) == false);
}

// ── toolbar_strip_click_outside (render_toolbar_strip site) ──────────────────
// Detects a click that landed outside both the strip and the hamburger button.
// That is the only hover-independent signal that closes the strip.

TEST_CASE("click_outside: left click outside window and button closes strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_click_outside(true, false) == true);
}

TEST_CASE("click_outside: left click inside strip window does not close", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_click_outside(true, true) == false);
}

TEST_CASE("click_outside: no click does not close", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_click_outside(false, false) == false);
    REQUIRE(toolbar_strip_click_outside(false, true) == false);
}
