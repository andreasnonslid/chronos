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
};
