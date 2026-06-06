#include <catch2/catch_test_macros.hpp>
#include "toolbar_strip.hpp"

// ── hover opens the strip ────────────────────────────────────────────────────

TEST_CASE("toolbar strip: button hover opens closed strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(false, false, true, false) == true);
}

TEST_CASE("toolbar strip: button hover keeps open strip open", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(true, false, true, false) == true);
}

// ── hovering the strip itself keeps it open ──────────────────────────────────

TEST_CASE("toolbar strip: strip hover keeps open strip open", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(true, false, false, true) == true);
}

TEST_CASE("toolbar strip: strip hover does not open closed strip", "[toolbar_strip]") {
    // strip_hovered can only be true when the strip was rendered (i.e. already open)
    // but we test it here anyway to pin the behaviour
    REQUIRE(toolbar_strip_next_state(false, false, false, true) == false);
}

// ── mouse-leave closes the strip ────────────────────────────────────────────

TEST_CASE("toolbar strip: no hover closes open strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(true, false, false, false) == false);
}

TEST_CASE("toolbar strip: no hover keeps closed strip closed", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(false, false, false, false) == false);
}

// ── click toggles (highest priority) ────────────────────────────────────────

TEST_CASE("toolbar strip: click opens closed strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(false, true, false, false) == true);
}

TEST_CASE("toolbar strip: click closes open strip even when hovered", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(true, true, true, true) == false);
}

TEST_CASE("toolbar strip: click closes open strip", "[toolbar_strip]") {
    REQUIRE(toolbar_strip_next_state(true, true, false, false) == false);
}
