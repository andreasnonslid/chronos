#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <chrono>
#include "formatting.hpp"
#include "timer_presets.hpp"

using namespace std::chrono;
using steady_duration = steady_clock::duration;

static steady_duration dur_ms(long long ms) { return duration_cast<steady_duration>(milliseconds(ms)); }

static steady_duration dur_s(long long s) { return duration_cast<steady_duration>(seconds(s)); }

// ── format_stopwatch_short / _long ──────────────────────────────────────────

TEST_CASE("format_stopwatch_short renders MM:SS.mmm across the supported range",
          "[formatting]") {
    struct Row { long long ms; std::wstring expected; };
    auto row = GENERATE(values<Row>({
        {0,                                  L"00:00.000"},
        {456,                                L"00:00.456"},
        {60'000,                             L"01:00.000"},
        {59 * 60'000 + 59'000 + 999,         L"59:59.999"}, // upper end of short range
    }));
    REQUIRE(format_stopwatch_short(dur_ms(row.ms)) == row.expected);
}

TEST_CASE("format_stopwatch_long renders HH:MM:SS.mmm across the supported range",
          "[formatting]") {
    struct Row { long long ms; std::wstring expected; };
    auto row = GENERATE(values<Row>({
        {0,                                  L"00:00:00.000"},
        {3'600'000,                          L"01:00:00.000"}, // 1h boundary
        {3'600'000 + 1'000 + 250,            L"01:00:01.250"}, // ms preserved
        {(3 * 3600 + 15 * 60 + 42) * 1'000,  L"03:15:42.000"}, // multi-hour
    }));
    REQUIRE(format_stopwatch_long(dur_ms(row.ms)) == row.expected);
}

// ── format_stopwatch_display ────────────────────────────────────────────────

TEST_CASE("format_stopwatch_display switches short/long at the 1h boundary",
          "[formatting]") {
    struct Row { long long ms; std::wstring expected; };
    auto row = GENERATE(values<Row>({
        {59 * 60'000 + 59'000 + 999, L"59:59.999"},     // just under 1h ⇒ short
        {3'600'000,                  L"01:00:00.000"},  // boundary       ⇒ long
        {3601'000 + 250,             L"01:00:01.250"},  // past 1h, ms kept
    }));
    REQUIRE(format_stopwatch_display(dur_ms(row.ms)) == row.expected);
}

// ── format_timer_display ────────────────────────────────────────────────────

TEST_CASE("format_timer_display: MM:SS below 1h, HH:MM:SS at and above", "[formatting]") {
    struct Row { long long secs; std::wstring expected; };
    auto row = GENERATE(values<Row>({
        {0,     L"00:00"},
        {1,     L"00:01"},
        {60,    L"01:00"},
        {3600,  L"01:00:00"}, // boundary: format switches at exactly 1h
        {86400, L"24:00:00"},
    }));
    REQUIRE(format_timer_display(dur_s(row.secs)) == row.expected);
}

// ── format_timer_edit ───────────────────────────────────────────────────────

TEST_CASE("format_timer_edit renders single-digit hour h:mm:ss", "[formatting]") {
    struct Row { long long secs; std::wstring expected; };
    auto row = GENERATE(values<Row>({
        {0,    L"0:00:00"},
        {60,   L"0:01:00"},
        {9000, L"2:30:00"},
    }));
    REQUIRE(format_timer_edit(dur_s(row.secs)) == row.expected);
}

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
// Left as individual cases: a previous GENERATE consolidation reliably failed
// on the Windows MSYS2/MinGW64 CI runner (clang21 + libstdc++ <format> with
// wchar_t) while passing everywhere else; rather than chase the platform
// quirk, keep each row in its own TEST_CASE so any future regression is local.

TEST_CASE("format_worked_time: zero", "[formatting]") {
    REQUIRE(format_worked_time(seconds{0}) == L"Worked: 0m");
}

TEST_CASE("format_worked_time: minutes only", "[formatting]") {
    REQUIRE(format_worked_time(seconds{25 * 60}) == L"Worked: 25m");
}

TEST_CASE("format_worked_time: sub-minute truncated", "[formatting]") {
    REQUIRE(format_worked_time(seconds{25 * 60 + 30}) == L"Worked: 25m");
}

TEST_CASE("format_worked_time: hours and minutes", "[formatting]") {
    REQUIRE(format_worked_time(seconds{75 * 60}) == L"Worked: 1h 15m");
}

TEST_CASE("format_worked_time: exact hours pad to two digits", "[formatting]") {
    REQUIRE(format_worked_time(seconds{2 * 3600}) == L"Worked: 2h 00m");
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

// ── format_timer_title / format_tray_title ────────────────────────────────

TEST_CASE("format_timer_title encodes label, position, and expired state",
          "[formatting]") {
    struct Row {
        const wchar_t* label; int idx; int total; const wchar_t* disp; bool expired;
        std::wstring expected;
    };
    auto row = GENERATE(values<Row>({
        {L"",    0, 1, L"04:32",    false, L"04:32"},
        {L"",    0, 1, L"00:00",    true,  L"EXPIRED 00:00"},
        {L"Tea", 1, 3, L"04:32",    false, L"Tea (2/3) 04:32"},
        {L"Tea", 1, 3, L"00:00",    true,  L"EXPIRED · Tea (2/3)"},
        {L"",    0, 2, L"01:00:00", false, L"Timer 1 (1/2) 01:00:00"},
        {L"",    2, 3, L"00:00",    true,  L"EXPIRED · Timer 3 (3/3)"},
    }));
    REQUIRE(format_timer_title(row.label, row.idx, row.total, row.disp, row.expired)
            == row.expected);
}

TEST_CASE("format_tray_title omits slot number for single timer, includes it otherwise",
          "[formatting]") {
    struct Row { int idx; int total; std::wstring expected; };
    auto row = GENERATE(values<Row>({
        {0, 1, L"Timer expired"},
        {0, 3, L"Timer 1 expired"},
        {2, 3, L"Timer 3 expired"},
    }));
    REQUIRE(format_tray_title(row.idx, row.total) == row.expected);
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
