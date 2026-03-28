#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include "../src/formatting.hpp"

using namespace std::chrono;
using steady_duration = steady_clock::duration;

static steady_duration dur_ms(long long ms) { return duration_cast<steady_duration>(milliseconds(ms)); }

static steady_duration dur_s(long long s) { return duration_cast<steady_duration>(seconds(s)); }

// ── format_stopwatch_short ──────────────────────────────────────────────────

TEST_CASE("format_stopwatch_short: zero duration", "[formatting]") {
    REQUIRE(format_stopwatch_short(steady_duration::zero()) == L"00:00.000");
}

TEST_CASE("format_stopwatch_short: sub-second", "[formatting]") {
    REQUIRE(format_stopwatch_short(dur_ms(456)) == L"00:00.456");
}

TEST_CASE("format_stopwatch_short: exact minute", "[formatting]") {
    REQUIRE(format_stopwatch_short(dur_s(60)) == L"01:00.000");
}

TEST_CASE("format_stopwatch_short: large value", "[formatting]") {
    // 59 minutes, 59 seconds, 999 ms
    REQUIRE(format_stopwatch_short(dur_ms(59 * 60 * 1000 + 59 * 1000 + 999)) == L"59:59.999");
}

// ── format_stopwatch_long ───────────────────────────────────────────────────

TEST_CASE("format_stopwatch_long: zero", "[formatting]") {
    REQUIRE(format_stopwatch_long(steady_duration::zero()) == L"00:00:00");
}

TEST_CASE("format_stopwatch_long: exactly 1 hour", "[formatting]") {
    REQUIRE(format_stopwatch_long(dur_s(3600)) == L"01:00:00");
}

TEST_CASE("format_stopwatch_long: multi-hour", "[formatting]") {
    REQUIRE(format_stopwatch_long(dur_s(3 * 3600 + 15 * 60 + 42)) == L"03:15:42");
}

// ── format_stopwatch_display ────────────────────────────────────────────────

TEST_CASE("format_stopwatch_display: under 1 hour uses short format", "[formatting]") {
    auto result = format_stopwatch_display(dur_ms(59 * 60 * 1000 + 59 * 1000 + 999));
    REQUIRE(result == L"59:59.999");
}

TEST_CASE("format_stopwatch_display: at 1 hour switches to long format", "[formatting]") {
    auto result = format_stopwatch_display(dur_s(3600));
    REQUIRE(result == L"01:00:00");
}

// ── format_timer_display ────────────────────────────────────────────────────

TEST_CASE("format_timer_display: zero", "[formatting]") {
    REQUIRE(format_timer_display(steady_duration::zero()) == L"00:00");
}

TEST_CASE("format_timer_display: 1 second", "[formatting]") { REQUIRE(format_timer_display(dur_s(1)) == L"00:01"); }

TEST_CASE("format_timer_display: 60 seconds", "[formatting]") { REQUIRE(format_timer_display(dur_s(60)) == L"01:00"); }

TEST_CASE("format_timer_display: >= 1 hour switches to HH:MM:SS", "[formatting]") {
    REQUIRE(format_timer_display(dur_s(3600)) == L"01:00:00");
    REQUIRE(format_timer_display(dur_s(86400)) == L"24:00:00");
}

// ── format_timer_edit ───────────────────────────────────────────────────────

TEST_CASE("format_timer_edit: zero", "[formatting]") {
    REQUIRE(format_timer_edit(steady_duration::zero()) == L"0:00:00");
}

TEST_CASE("format_timer_edit: 1 minute", "[formatting]") { REQUIRE(format_timer_edit(dur_s(60)) == L"0:01:00"); }

TEST_CASE("format_timer_edit: 2.5 hours", "[formatting]") { REQUIRE(format_timer_edit(dur_s(9000)) == L"2:30:00"); }

// ── format_lap_row ──────────────────────────────────────────────────────────

TEST_CASE("format_lap_row: lap 1", "[formatting]") {
    auto result = format_lap_row(1, dur_ms(5123), dur_ms(5123));
    REQUIRE(result.find(L"Lap 1") != std::wstring::npos);
    REQUIRE(result.find(L"00:05.123") != std::wstring::npos);
}

TEST_CASE("format_lap_row: large lap number", "[formatting]") {
    auto result = format_lap_row(100, dur_ms(3456), dur_ms(80000));
    REQUIRE(result.find(L"Lap 100") != std::wstring::npos);
}
