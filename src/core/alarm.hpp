#pragma once
#include <string>

// Day bitmask: bit 0 = Monday, bit 1 = Tuesday, ..., bit 6 = Sunday
inline constexpr int ALARM_DAY_MON = 1 << 0;
inline constexpr int ALARM_DAY_TUE = 1 << 1;
inline constexpr int ALARM_DAY_WED = 1 << 2;
inline constexpr int ALARM_DAY_THU = 1 << 3;
inline constexpr int ALARM_DAY_FRI = 1 << 4;
inline constexpr int ALARM_DAY_SAT = 1 << 5;
inline constexpr int ALARM_DAY_SUN = 1 << 6;
inline constexpr int ALARM_ALL_DAYS = 0x7F;
inline constexpr int ALARM_WEEKDAYS = 0x1F;
inline constexpr int ALARM_WEEKEND  = 0x60;

inline constexpr int ALARM_MAX_COUNT = 20;

enum class AlarmSchedule { Days = 0, Date = 1 };

struct Alarm {
    std::string name;
    AlarmSchedule schedule = AlarmSchedule::Days;
    int days_mask = ALARM_ALL_DAYS;
    int date_year = 2026, date_month = 1, date_day = 1;
    int hour = 8, minute = 0;
    bool enabled = true;
    bool notified = false;  // runtime-only, not persisted; reset each minute by check_alarms
};

// Pure decision: should alarm a fire at the given wall-clock moment?
// dow_posix: POSIX tm_wday (0=Sunday, 1=Monday, …, 6=Saturday)
inline bool alarm_matches(const Alarm& a,
                          int h, int m,
                          int dow_posix,
                          int year, int month, int day) {
    if (a.hour != h || a.minute != m) return false;
    if (a.schedule == AlarmSchedule::Days) {
        // Convert POSIX dow to bitmask index (Mon=0 … Sun=6)
        int bit = (dow_posix == 0) ? 6 : (dow_posix - 1);
        return (a.days_mask & (1 << bit)) != 0;
    }
    return a.date_year == year && a.date_month == month && a.date_day == day;
}
