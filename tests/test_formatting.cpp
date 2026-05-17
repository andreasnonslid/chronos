#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <chrono>
#include "formatting.hpp"
#include "timer_presets.hpp"

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
    REQUIRE(format_stopwatch_long(steady_duration::zero()) == L"00:00:00.000");
}

TEST_CASE("format_stopwatch_long: exactly 1 hour", "[formatting]") {
    REQUIRE(format_stopwatch_long(dur_s(3600)) == L"01:00:00.000");
}

TEST_CASE("format_stopwatch_long: multi-hour", "[formatting]") {
    REQUIRE(format_stopwatch_long(dur_s(3 * 3600 + 15 * 60 + 42)) == L"03:15:42.000");
}

TEST_CASE("format_stopwatch_long: preserves milliseconds", "[formatting]") {
    REQUIRE(format_stopwatch_long(dur_ms(3600 * 1000 + 1 * 1000 + 250)) == L"01:00:01.250");
}

// ── format_stopwatch_display ────────────────────────────────────────────────

TEST_CASE("format_stopwatch_display: under 1 hour uses short format", "[formatting]") {
    auto result = format_stopwatch_display(dur_ms(59 * 60 * 1000 + 59 * 1000 + 999));
    REQUIRE(result == L"59:59.999");
}

TEST_CASE("format_stopwatch_display: at 1 hour switches to long format with ms", "[formatting]") {
    auto result = format_stopwatch_display(dur_s(3600));
    REQUIRE(result == L"01:00:00.000");
}

TEST_CASE("format_stopwatch_display: past 1 hour preserves milliseconds", "[formatting]") {
    auto result = format_stopwatch_display(dur_ms(3601 * 1000 + 250));
    REQUIRE(result == L"01:00:01.250");
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

// ── format_worked_time ─────────────────────────────────────────────────────

TEST_CASE("format_worked_time: zero minutes", "[formatting]") {
    REQUIRE(format_worked_time(seconds{0}) == L"Worked: 0m");
}

TEST_CASE("format_worked_time: minutes only", "[formatting]") {
    REQUIRE(format_worked_time(seconds{25 * 60}) == L"Worked: 25m");
}

TEST_CASE("format_worked_time: hours and minutes", "[formatting]") {
    REQUIRE(format_worked_time(seconds{75 * 60}) == L"Worked: 1h 15m");
}

TEST_CASE("format_worked_time: exact hours", "[formatting]") {
    REQUIRE(format_worked_time(seconds{2 * 3600}) == L"Worked: 2h 00m");
}

TEST_CASE("format_worked_time: partial seconds truncated to minutes", "[formatting]") {
    REQUIRE(format_worked_time(seconds{25 * 60 + 30}) == L"Worked: 25m");
}

// ── negative duration clamping ──────────────────────────────────────────────

// Every duration formatter must clamp negative inputs to its zero string,
// and every clamp must hold across a range of negative values.
TEST_CASE("formatting: given negative duration when any duration formatter is called"
          " then output equals the formatter's zero string",
          "[formatting]") {
    auto neg_ms = GENERATE(-1, -500, -3600 * 1000, -86400 * 1000);
    REQUIRE(format_stopwatch_short(dur_ms(neg_ms)) == L"00:00.000");
    REQUIRE(format_stopwatch_long(dur_ms(neg_ms))  == L"00:00:00.000");
    REQUIRE(format_timer_display(dur_ms(neg_ms))   == L"00:00");
    REQUIRE(format_timer_edit(dur_ms(neg_ms))      == L"0:00:00");
}

// ── format_timer_title ────────────────────────────────────────────────────

TEST_CASE("format_timer_title: single timer running unchanged", "[formatting]") {
    REQUIRE(format_timer_title(L"", 0, 1, L"04:32", false) == L"04:32");
}

TEST_CASE("format_timer_title: single timer expired unchanged", "[formatting]") {
    REQUIRE(format_timer_title(L"", 0, 1, L"00:00", true) == L"EXPIRED 00:00");
}

TEST_CASE("format_timer_title: multi-timer running with label", "[formatting]") {
    REQUIRE(format_timer_title(L"Tea", 1, 3, L"04:32", false) == L"Tea (2/3) 04:32");
}

TEST_CASE("format_timer_title: multi-timer expired with label", "[formatting]") {
    REQUIRE(format_timer_title(L"Tea", 1, 3, L"00:00", true) == L"EXPIRED · Tea (2/3)");
}

TEST_CASE("format_timer_title: multi-timer running no label", "[formatting]") {
    REQUIRE(format_timer_title(L"", 0, 2, L"01:00:00", false) == L"Timer 1 (1/2) 01:00:00");
}

TEST_CASE("format_timer_title: multi-timer expired no label", "[formatting]") {
    REQUIRE(format_timer_title(L"", 2, 3, L"00:00", true) == L"EXPIRED · Timer 3 (3/3)");
}

// ── format_tray_title ─────────────────────────────────────────────────────

TEST_CASE("format_tray_title: single timer", "[formatting]") {
    REQUIRE(format_tray_title(0, 1) == L"Timer expired");
}

TEST_CASE("format_tray_title: multi-timer slot 1", "[formatting]") {
    REQUIRE(format_tray_title(0, 3) == L"Timer 1 expired");
}

TEST_CASE("format_tray_title: multi-timer slot 3", "[formatting]") {
    REQUIRE(format_tray_title(2, 3) == L"Timer 3 expired");
}

// ── format_preset_label ──────────────────────────────────────────────────

// Table-driven: enumerate every shape the preset formatter must handle
// (exact minutes, mm:ss, hh:mm:ss with and without seconds) so the format
// contract lives in one place.
TEST_CASE("formatting: format_preset_label renders the expected mm:ss / h:mm:ss shape"
          " for each preset duration",
          "[formatting]") {
    struct Row { int secs; std::wstring expected; };
    auto row = GENERATE(values<Row>({
        {60,   L"1:00"},
        {300,  L"5:00"},
        {2700, L"45:00"},
        {90,   L"1:30"},
        {450,  L"7:30"},
        {3600, L"1:00:00"},
        {5400, L"1:30:00"},
        {3661, L"1:01:01"},
    }));
    REQUIRE(format_preset_label(row.secs) == row.expected);
}
