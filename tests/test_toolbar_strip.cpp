#include <catch2/catch_test_macros.hpp>
#include "toolbar_strip.hpp"

// ── toolbar_strip_btn_update (render_titlebar site) ──────────────────────────
// Handles click-toggle and hover-open. Never closes the strip — that is the
// responsibility of toolbar_strip_should_stay_open at the strip render site.

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

// ── toolbar_strip_should_stay_open (render_toolbar_strip site) ───────────────
// Called inside the strip's Begin/End with current-frame values for both
// the button hover (from UiState, written earlier this same frame) and the
// strip's own IsWindowHovered(). Returns false → caller sets strip_open=false.

TEST_CASE("should_stay_open: returns true when button is hovered", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_should_stay_open(true, false) == true);
}

TEST_CASE("should_stay_open: returns true when strip is hovered", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_should_stay_open(false, true) == true);
}

TEST_CASE("should_stay_open: returns true when both hovered", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_should_stay_open(true, true) == true);
}

TEST_CASE("should_stay_open: returns false when neither hovered", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_should_stay_open(false, false) == false);
}
