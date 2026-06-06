#include <catch2/catch_test_macros.hpp>
#include "codicons/codicons.h"

// Decode the first codepoint from a UTF-8 string literal (handles 1–3-byte sequences).
static constexpr unsigned decode_first_cp(const char* s) noexcept {
    auto b = [s](int i) { return static_cast<unsigned char>(s[i]); };
    if ((b(0) & 0xF0) == 0xE0)
        return ((b(0) & 0x0Fu) << 12u) | ((b(1) & 0x3Fu) << 6u) | (b(2) & 0x3Fu);
    if ((b(0) & 0xE0) == 0xC0)
        return ((b(0) & 0x1Fu) <<  6u) |  (b(1) & 0x3Fu);
    return b(0);
}

// Loaded glyph range (must match the icon_ranges[] constant in main_loop.cpp).
static constexpr unsigned RANGE_LO = 0xEA00u;
static constexpr unsigned RANGE_HI = 0xECFFu;

static constexpr bool in_range(const char* icon) noexcept {
    unsigned cp = decode_first_cp(icon);
    return cp >= RANGE_LO && cp <= RANGE_HI;
}

// ── ICON_CLOCKFACE is the key icon called out in issue #530 ──────────────────

TEST_CASE("codicon range covers ICON_CLOCKFACE (U+EC75)", "[codicon]") {
    // ICON_CLOCKFACE is at U+EC75 — near the top of the block.
    // A range capped at 0xEC00 would silently drop it.
    REQUIRE(decode_first_cp(ICON_CLOCKFACE) == 0xEC75u);
    REQUIRE(in_range(ICON_CLOCKFACE));
}

// ── All icons used in the titlebar buttons ───────────────────────────────────

TEST_CASE("codicon range covers titlebar icons", "[codicon]") {
    CHECK(in_range(ICON_MENU));
    CHECK(in_range(ICON_SETTINGS_GEAR));
    CHECK(in_range(ICON_CHROME_MINIMIZE));
    CHECK(in_range(ICON_CHROME_CLOSE));
}

// ── All icons used in the toolbar strip ─────────────────────────────────────

TEST_CASE("codicon range covers toolbar strip icons", "[codicon]") {
    CHECK(in_range(ICON_PIN));
    CHECK(in_range(ICON_CLOCKFACE));
    CHECK(in_range(ICON_HISTORY));
    CHECK(in_range(ICON_WATCH));
    CHECK(in_range(ICON_BELL));
}
