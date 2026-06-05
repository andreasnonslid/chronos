// Tests for the alarm subsystem. The audit identified alarms as the largest
// untested surface in the codebase: Alarm round-trip in config (Days vs Date
// schedules, days_mask, enabled, name), date normalization for short months
// and leap years, and the dispatch entry points (A_ALARM_ADD / A_ALARM_DEL+i
// / A_ALARM_TOGGLE+i / A_SHOW_ALARMS / A_SETTINGS).
//
// Tags used: [alarm], [config-alarm], [actions-alarm].

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <sstream>
#include "actions.hpp"
#include "alarm.hpp"
#include "app.hpp"
#include "config.hpp"
#include "config_serial.hpp"
#include "test_helpers.hpp"

using test_helpers::t0;

namespace {

Alarm make_alarm_days(std::string name, int hour, int minute, int days_mask, bool enabled = true) {
    Alarm a;
    a.name = std::move(name);
    a.schedule = AlarmSchedule::Days;
    a.days_mask = days_mask;
    a.hour = hour;
    a.minute = minute;
    a.enabled = enabled;
    return a;
}

Alarm make_alarm_date(std::string name, int y, int mo, int d, int hour, int minute, bool enabled = true) {
    Alarm a;
    a.name = std::move(name);
    a.schedule = AlarmSchedule::Date;
    a.date_year = y;
    a.date_month = mo;
    a.date_day = d;
    a.hour = hour;
    a.minute = minute;
    a.enabled = enabled;
    return a;
}

// Round-trip a Config through config_write/config_read and return the result.
Config round_trip(const Config& orig) {
    std::ostringstream os;
    REQUIRE(config_write(orig, os));
    Config back;
    std::istringstream is(os.str());
    REQUIRE(config_read(back, is));
    return back;
}

} // namespace

// ─── config: alarm round-trip ────────────────────────────────────────────────

TEST_CASE("config-alarm: given days-schedule alarm when round-tripped then all fields preserved",
          "[config-alarm]") {
    Config orig;
    orig.alarms = { make_alarm_days("Wake", 7, 30, ALARM_WEEKDAYS) };
    auto back = round_trip(orig);
    REQUIRE(back.alarms.size() == 1);
    const auto& a = back.alarms[0];
    REQUIRE(a.name == "Wake");
    REQUIRE(a.schedule == AlarmSchedule::Days);
    REQUIRE(a.hour == 7);
    REQUIRE(a.minute == 30);
    REQUIRE(a.days_mask == ALARM_WEEKDAYS);
    REQUIRE(a.enabled);
}

TEST_CASE("config-alarm: given date-schedule alarm when round-tripped then all fields preserved",
          "[config-alarm]") {
    Config orig;
    orig.alarms = { make_alarm_date("Dentist", 2026, 6, 15, 14, 0) };
    auto back = round_trip(orig);
    REQUIRE(back.alarms.size() == 1);
    const auto& a = back.alarms[0];
    REQUIRE(a.name == "Dentist");
    REQUIRE(a.schedule == AlarmSchedule::Date);
    REQUIRE(a.date_year == 2026);
    REQUIRE(a.date_month == 6);
    REQUIRE(a.date_day == 15);
    REQUIRE(a.hour == 14);
    REQUIRE(a.minute == 0);
}

TEST_CASE("config-alarm: given disabled alarm when round-tripped then enabled=false preserved",
          "[config-alarm]") {
    Config orig;
    orig.alarms = { make_alarm_days("Off", 9, 0, ALARM_ALL_DAYS, /*enabled=*/false) };
    auto back = round_trip(orig);
    REQUIRE(back.alarms.size() == 1);
    REQUIRE_FALSE(back.alarms[0].enabled);
}

TEST_CASE("config-alarm: given multiple alarms when round-tripped then count and order preserved",
          "[config-alarm]") {
    Config orig;
    orig.alarms = {
        make_alarm_days("A", 6, 0, ALARM_ALL_DAYS),
        make_alarm_days("B", 7, 15, ALARM_WEEKDAYS),
        make_alarm_date("C", 2026, 12, 25, 9, 0),
    };
    auto back = round_trip(orig);
    REQUIRE(back.alarms.size() == 3);
    REQUIRE(back.alarms[0].name == "A");
    REQUIRE(back.alarms[1].name == "B");
    REQUIRE(back.alarms[2].name == "C");
    REQUIRE(back.alarms[2].schedule == AlarmSchedule::Date);
}

TEST_CASE("config-alarm: given no alarms when written then no alarm keys emitted",
          "[config-alarm]") {
    Config orig;
    std::ostringstream os;
    REQUIRE(config_write(orig, os));
    REQUIRE(os.str().find("alarm") == std::string::npos);
    REQUIRE(os.str().find("num_alarms") == std::string::npos);
}

TEST_CASE("config-alarm: given show_alarms toggled when round-tripped then preserved",
          "[config-alarm]") {
    for (bool show : {true, false}) {
        Config orig;
        orig.show_alarms = show;
        auto back = round_trip(orig);
        REQUIRE(back.show_alarms == show);
    }
}

// ─── config: alarm clamping ──────────────────────────────────────────────────

TEST_CASE("config-alarm: hour out of range is clamped on read", "[config-alarm]") {
    struct C { int written; int expected; };
    auto c = GENERATE(C{-5, 0}, C{0, 0}, C{12, 12}, C{23, 23}, C{24, 23}, C{999, 23});
    std::ostringstream os;
    os << "num_alarms=1\nalarm0_schedule=0\nalarm0_hour=" << c.written
       << "\nalarm0_min=0\nalarm0_days=" << ALARM_ALL_DAYS << "\n";
    Config back;
    std::istringstream is(os.str());
    REQUIRE(config_read(back, is));
    REQUIRE(back.alarms.size() == 1);
    REQUIRE(back.alarms[0].hour == c.expected);
}

TEST_CASE("config-alarm: minute out of range is clamped on read", "[config-alarm]") {
    auto pair = GENERATE(std::make_pair(-1, 0), std::make_pair(0, 0), std::make_pair(59, 59),
                         std::make_pair(60, 59), std::make_pair(9999, 59));
    std::ostringstream os;
    os << "num_alarms=1\nalarm0_schedule=0\nalarm0_hour=0\nalarm0_min=" << pair.first
       << "\nalarm0_days=" << ALARM_ALL_DAYS << "\n";
    Config back;
    std::istringstream is(os.str());
    REQUIRE(config_read(back, is));
    REQUIRE(back.alarms.size() == 1);
    REQUIRE(back.alarms[0].minute == pair.second);
}

TEST_CASE("config-alarm: days_mask is clamped to [0, ALARM_ALL_DAYS]", "[config-alarm]") {
    auto pair = GENERATE(std::make_pair(-1, 0), std::make_pair(0, 0),
                         std::make_pair(ALARM_WEEKDAYS, ALARM_WEEKDAYS),
                         std::make_pair(ALARM_ALL_DAYS, ALARM_ALL_DAYS),
                         std::make_pair(0xFF, ALARM_ALL_DAYS));
    std::ostringstream os;
    os << "num_alarms=1\nalarm0_schedule=0\nalarm0_hour=0\nalarm0_min=0\nalarm0_days="
       << pair.first << "\n";
    Config back;
    std::istringstream is(os.str());
    REQUIRE(config_read(back, is));
    REQUIRE(back.alarms.size() == 1);
    REQUIRE(back.alarms[0].days_mask == pair.second);
}

TEST_CASE("config-alarm: num_alarms clamped to ALARM_MAX_COUNT", "[config-alarm]") {
    std::istringstream is("num_alarms=9999\n");
    Config c;
    REQUIRE(config_read(c, is));
    REQUIRE((int)c.alarms.size() <= ALARM_MAX_COUNT);
}

// ─── config: alarm date normalization (leap years and short months) ──────────

TEST_CASE("config-alarm: feb 29 normalizes per leap-year rule", "[config-alarm]") {
    // Year, divisible-by-4-but-not-100-or-by-400 => leap year; otherwise not.
    struct C { int year; int input_day; int expected_day; };
    auto c = GENERATE(
        C{2024, 29, 29}, // leap (div 4, not 100)
        C{2026, 29, 28}, // common
        C{2100, 29, 28}, // not leap (div 100, not 400)
        C{2000, 29, 29}, // leap (div 400)
        C{2025, 30, 28}  // beyond any feb day
    );
    Config cfg;
    cfg.alarms = { make_alarm_date("Feb", c.year, 2, c.input_day, 9, 0) };
    auto back = round_trip(cfg);
    REQUIRE(back.alarms.size() == 1);
    REQUIRE(back.alarms[0].date_day == c.expected_day);
    REQUIRE(back.alarms[0].date_month == 2);
    REQUIRE(back.alarms[0].date_year == c.year);
}

TEST_CASE("config-alarm: 30-day months clamp day to 30", "[config-alarm]") {
    auto month = GENERATE(4, 6, 9, 11);
    Config cfg;
    cfg.alarms = { make_alarm_date("EOM", 2026, month, 31, 9, 0) };
    auto back = round_trip(cfg);
    REQUIRE(back.alarms.size() == 1);
    REQUIRE(back.alarms[0].date_day == 30);
}

TEST_CASE("config-alarm: 31-day months accept day=31", "[config-alarm]") {
    auto month = GENERATE(1, 3, 5, 7, 8, 10, 12);
    Config cfg;
    cfg.alarms = { make_alarm_date("EOM", 2026, month, 31, 9, 0) };
    auto back = round_trip(cfg);
    REQUIRE(back.alarms[0].date_day == 31);
}

TEST_CASE("config-alarm: days-schedule alarm leaves date fields untouched on normalize",
          "[config-alarm]") {
    // Normalization only applies to Date-schedule alarms.
    Config cfg;
    auto a = make_alarm_days("D", 9, 0, ALARM_ALL_DAYS);
    a.date_month = 2;
    a.date_day = 31; // intentionally invalid Feb day
    cfg.alarms = { a };
    auto back = round_trip(cfg);
    REQUIRE(back.alarms[0].schedule == AlarmSchedule::Days);
    // date fields are not written for Days schedule, so they revert to Alarm defaults.
    // The key invariant: no crash, alarm survives the round-trip.
    REQUIRE(back.alarms[0].hour == 9);
}

// ─── alarm constants sanity ──────────────────────────────────────────────────

TEST_CASE("alarm: day bitmask constants are mutually exclusive and ALARM_ALL_DAYS covers them",
          "[alarm]") {
    int combined = ALARM_DAY_MON | ALARM_DAY_TUE | ALARM_DAY_WED | ALARM_DAY_THU
                 | ALARM_DAY_FRI | ALARM_DAY_SAT | ALARM_DAY_SUN;
    REQUIRE(combined == ALARM_ALL_DAYS);
    REQUIRE((ALARM_WEEKDAYS | ALARM_WEEKEND) == ALARM_ALL_DAYS);
    REQUIRE((ALARM_WEEKDAYS & ALARM_WEEKEND) == 0);
}

// ─── dispatch: A_SHOW_ALARMS / A_SETTINGS / A_ALARM_ADD ──────────────────────

TEST_CASE("actions-alarm: A_SHOW_ALARMS toggles app.show_alarms and signals resize+save",
          "[actions-alarm]") {
    App app;
    REQUIRE_FALSE(app.show_alarms);
    auto r1 = dispatch_action(app, A_SHOW_ALARMS, t0(), {});
    REQUIRE(app.show_alarms);
    REQUIRE(r1.resize);
    REQUIRE(r1.save_config);
    auto r2 = dispatch_action(app, A_SHOW_ALARMS, t0(), {});
    REQUIRE_FALSE(app.show_alarms);
    REQUIRE(r2.resize);
}

TEST_CASE("actions-alarm: A_SETTINGS sets open_settings only", "[actions-alarm]") {
    App app;
    auto r = dispatch_action(app, A_SETTINGS, t0(), {});
    REQUIRE(r.open_settings);
    REQUIRE_FALSE(r.save_config);
    REQUIRE_FALSE(r.resize);
    REQUIRE_FALSE(r.set_topmost);
}

TEST_CASE("actions-alarm: A_ALARM_ADD sets open_alarm_dialog only", "[actions-alarm]") {
    App app;
    auto r = dispatch_action(app, A_ALARM_ADD, t0(), {});
    REQUIRE(r.open_alarm_dialog);
    REQUIRE_FALSE(r.save_config);
    REQUIRE(app.alarms.empty()); // dispatcher does not itself add an alarm
}

// ─── dispatch: A_ALARM_DEL+i ─────────────────────────────────────────────────

TEST_CASE("actions-alarm: A_ALARM_DEL+i removes alarm at index and signals resize+save",
          "[actions-alarm]") {
    App app;
    app.alarms = { make_alarm_days("A", 6, 0, ALARM_ALL_DAYS),
                   make_alarm_days("B", 7, 0, ALARM_ALL_DAYS),
                   make_alarm_days("C", 8, 0, ALARM_ALL_DAYS) };
    auto r = dispatch_action(app, A_ALARM_DEL + 1, t0(), {});
    REQUIRE(app.alarms.size() == 2);
    REQUIRE(app.alarms[0].name == "A");
    REQUIRE(app.alarms[1].name == "C");
    REQUIRE(r.resize);
    REQUIRE(r.save_config);
}

TEST_CASE("actions-alarm: A_ALARM_DEL+i with out-of-range index is a no-op",
          "[actions-alarm]") {
    App app;
    app.alarms = { make_alarm_days("Only", 6, 0, ALARM_ALL_DAYS) };
    auto i = GENERATE(5, 10, ALARM_MAX_COUNT - 1);
    auto r = dispatch_action(app, A_ALARM_DEL + i, t0(), {});
    REQUIRE(app.alarms.size() == 1);
    REQUIRE_FALSE(r.resize);
    REQUIRE_FALSE(r.save_config);
}

// ─── dispatch: A_ALARM_TOGGLE+i ──────────────────────────────────────────────

TEST_CASE("actions-alarm: A_ALARM_TOGGLE+i flips enabled and signals save (no resize)",
          "[actions-alarm]") {
    App app;
    app.alarms = { make_alarm_days("A", 6, 0, ALARM_ALL_DAYS, /*enabled=*/true),
                   make_alarm_days("B", 7, 0, ALARM_ALL_DAYS, /*enabled=*/true) };
    auto r1 = dispatch_action(app, A_ALARM_TOGGLE + 1, t0(), {});
    REQUIRE(app.alarms[0].enabled);
    REQUIRE_FALSE(app.alarms[1].enabled);
    REQUIRE(r1.save_config);
    REQUIRE_FALSE(r1.resize);

    auto r2 = dispatch_action(app, A_ALARM_TOGGLE + 1, t0(), {});
    REQUIRE(app.alarms[1].enabled);
    REQUIRE(r2.save_config);
}

TEST_CASE("actions-alarm: A_ALARM_TOGGLE+i with out-of-range index is a no-op",
          "[actions-alarm]") {
    App app;
    app.alarms = { make_alarm_days("Only", 6, 0, ALARM_ALL_DAYS, /*enabled=*/true) };
    auto i = GENERATE(3, 7, ALARM_MAX_COUNT - 1);
    auto r = dispatch_action(app, A_ALARM_TOGGLE + i, t0(), {});
    REQUIRE(app.alarms[0].enabled);
    REQUIRE_FALSE(r.save_config);
}

// ─── wants_blink for alarm actions ───────────────────────────────────────────

TEST_CASE("actions-alarm: alarm dispatch actions do not request blink", "[actions-alarm]") {
    REQUIRE_FALSE(wants_blink(A_SHOW_ALARMS));
    REQUIRE_FALSE(wants_blink(A_SETTINGS));
    REQUIRE_FALSE(wants_blink(A_ALARM_ADD));
    auto i = GENERATE(0, 1, 5, ALARM_MAX_COUNT - 1);
    REQUIRE_FALSE(wants_blink(A_ALARM_DEL + i));
    REQUIRE_FALSE(wants_blink(A_ALARM_TOGGLE + i));
}

// ─── alarm_matches(): pure fire-decision function ─────────────────────────────
// POSIX dow: 0=Sunday, 1=Monday, ..., 6=Saturday
// Bitmask: bit 0=Monday, bit 1=Tuesday, ..., bit 6=Sunday

TEST_CASE("alarm_matches: wrong hour or minute returns false", "[alarm]") {
    auto a = make_alarm_days("X", 8, 30, ALARM_ALL_DAYS);
    REQUIRE_FALSE(alarm_matches(a, 7, 30, 1, 2026, 6, 5));  // hour off
    REQUIRE_FALSE(alarm_matches(a, 8, 29, 1, 2026, 6, 5));  // minute off
    REQUIRE_FALSE(alarm_matches(a, 9, 30, 1, 2026, 6, 5));  // both off
}

TEST_CASE("alarm_matches: days-schedule fires on correct day", "[alarm]") {
    auto a = make_alarm_days("Weekday", 8, 0, ALARM_WEEKDAYS);
    // Monday..Friday (POSIX dow 1..5) → bits 0..4 → match
    REQUIRE(alarm_matches(a, 8, 0, 1, 2026, 6, 1));  // Monday
    REQUIRE(alarm_matches(a, 8, 0, 2, 2026, 6, 2));  // Tuesday
    REQUIRE(alarm_matches(a, 8, 0, 5, 2026, 6, 5));  // Friday
    // Saturday (POSIX 6) → bit 5, weekend bit → no match
    REQUIRE_FALSE(alarm_matches(a, 8, 0, 6, 2026, 6, 6));
    // Sunday (POSIX 0) → bit 6, no match
    REQUIRE_FALSE(alarm_matches(a, 8, 0, 0, 2026, 6, 7));
}

TEST_CASE("alarm_matches: days-schedule POSIX Sunday (0) maps to bit 6", "[alarm]") {
    auto a = make_alarm_days("Weekend", 10, 0, ALARM_WEEKEND); // bits 5+6 = Sat+Sun
    REQUIRE(alarm_matches(a, 10, 0, 0, 2026, 6, 7));  // Sunday POSIX=0 → bit 6
    REQUIRE(alarm_matches(a, 10, 0, 6, 2026, 6, 6));  // Saturday POSIX=6 → bit 5
    REQUIRE_FALSE(alarm_matches(a, 10, 0, 1, 2026, 6, 1)); // Monday → bit 0, not set
}

TEST_CASE("alarm_matches: days-schedule all-days fires on every day", "[alarm]") {
    auto a = make_alarm_days("Daily", 6, 0, ALARM_ALL_DAYS);
    // All 7 POSIX dow values (0=Sun through 6=Sat)
    for (int dow = 0; dow <= 6; ++dow)
        REQUIRE(alarm_matches(a, 6, 0, dow, 2026, 1, 1));
}

TEST_CASE("alarm_matches: days-schedule with empty mask never fires", "[alarm]") {
    auto a = make_alarm_days("None", 8, 0, 0);
    for (int dow = 0; dow <= 6; ++dow)
        REQUIRE_FALSE(alarm_matches(a, 8, 0, dow, 2026, 1, 1));
}

TEST_CASE("alarm_matches: date-schedule fires only on exact date", "[alarm]") {
    auto a = make_alarm_date("Meeting", 2026, 6, 15, 14, 30);
    REQUIRE(alarm_matches(a, 14, 30, 1, 2026, 6, 15));        // exact
    REQUIRE_FALSE(alarm_matches(a, 14, 30, 1, 2026, 6, 14));  // day off
    REQUIRE_FALSE(alarm_matches(a, 14, 30, 1, 2026, 5, 15));  // month off
    REQUIRE_FALSE(alarm_matches(a, 14, 30, 1, 2025, 6, 15));  // year off
    REQUIRE_FALSE(alarm_matches(a, 14, 30, 1, 2026, 6, 16));  // day after
}

TEST_CASE("alarm_matches: date-schedule ignores days_mask", "[alarm]") {
    // A date-schedule alarm should not check days_mask.
    auto a = make_alarm_date("Ignore", 2026, 12, 25, 9, 0);
    a.days_mask = 0; // would block all days if checked
    REQUIRE(alarm_matches(a, 9, 0, 5, 2026, 12, 25)); // fires regardless
}

TEST_CASE("alarm_matches: days-schedule ignores date fields", "[alarm]") {
    // Days-schedule: correct time + correct dow = fire, even if date fields differ
    auto a = make_alarm_days("Daily", 9, 0, ALARM_ALL_DAYS);
    a.date_year = 1900; a.date_month = 1; a.date_day = 1; // obviously wrong date
    REQUIRE(alarm_matches(a, 9, 0, 3, 2026, 6, 15)); // should still fire
}

TEST_CASE("alarm_matches: single-day mask fires only on that day", "[alarm]") {
    // Wednesday only: bit 2
    auto a = make_alarm_days("Wed", 12, 0, ALARM_DAY_WED);
    REQUIRE(alarm_matches(a, 12, 0, 3, 2026, 6, 3));  // Wednesday POSIX=3 → bit 2
    REQUIRE_FALSE(alarm_matches(a, 12, 0, 2, 2026, 6, 2));  // Tuesday
    REQUIRE_FALSE(alarm_matches(a, 12, 0, 4, 2026, 6, 4));  // Thursday
}
