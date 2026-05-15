#include "config_serial.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <format>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include "config.hpp"

namespace {

inline int clamp_int(long long v, int lo, int hi) {
    return (int)std::clamp(v, (long long)lo, (long long)hi);
}

// Table-driven AnalogClockStyle int fields. NO_CLAMP_* means "accept any value"
// (used for raw color codes where -1 = "fall back to theme").
constexpr int NO_CLAMP_LO = INT_MIN;
constexpr int NO_CLAMP_HI = INT_MAX;

struct AnalogInt {
    std::string_view key;
    int AnalogClockStyle::*field;
    int lo, hi;
};

constexpr std::array<AnalogInt, 25> kAnalogInts = {{
    {"analog_hour_color",         &AnalogClockStyle::hour_color,         NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_minute_color",       &AnalogClockStyle::minute_color,       NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_second_color",       &AnalogClockStyle::second_color,       NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_face_color",         &AnalogClockStyle::face_color,         NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_tick_color",         &AnalogClockStyle::tick_color,         NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_background_color",   &AnalogClockStyle::background_color,   NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_face_fill_color",    &AnalogClockStyle::face_fill_color,    NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_face_outline_color", &AnalogClockStyle::face_outline_color, NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_hour_label_color",   &AnalogClockStyle::hour_label_color,   NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_center_dot_color",   &AnalogClockStyle::center_dot_color,   NO_CLAMP_LO, NO_CLAMP_HI},
    {"analog_hour_len",           &AnalogClockStyle::hour_len_pct,        0,  100},
    {"analog_minute_len",         &AnalogClockStyle::minute_len_pct,      0,  100},
    {"analog_second_len",         &AnalogClockStyle::second_len_pct,      0,  100},
    {"analog_hour_thick",         &AnalogClockStyle::hour_thickness,      1,    6},
    {"analog_minute_thick",       &AnalogClockStyle::minute_thickness,    1,    4},
    {"analog_second_thick",       &AnalogClockStyle::second_thickness,    1,    3},
    {"analog_hour_tick",          &AnalogClockStyle::hour_tick_pct,       5,   20},
    {"analog_minute_tick",        &AnalogClockStyle::minute_tick_pct,     2,   10},
    {"analog_center_dot_size",    &AnalogClockStyle::center_dot_size,     0,   10},
    {"analog_hour_opacity",       &AnalogClockStyle::hour_opacity_pct,   10,  100},
    {"analog_minute_opacity",     &AnalogClockStyle::minute_opacity_pct, 10,  100},
    {"analog_second_opacity",     &AnalogClockStyle::second_opacity_pct, 10,  100},
    {"analog_tick_opacity",       &AnalogClockStyle::tick_opacity_pct,   10,  100},
    {"analog_face_opacity",       &AnalogClockStyle::face_opacity_pct,    0,  100},
    {"analog_radius",             &AnalogClockStyle::radius_pct,         50,  100},
}};
// hour_labels (enum) and show_minute_ticks (bool) handled inline.

// Per-timer-slot field tables. Each timer i may have suffixed keys like
// "timer3_elapsed_ms" that map onto an array<long long/bool, MAX_TIMERS>.
struct TimerSlotI64  { std::string_view suffix; std::array<long long, Config::MAX_TIMERS> Config::*field; };
struct TimerSlotBool { std::string_view suffix; std::array<bool,      Config::MAX_TIMERS> Config::*field; };

constexpr std::array<TimerSlotI64, 3> kTimerI64 = {{
    {"_elapsed_ms",         &Config::timer_elapsed_ms},
    {"_start_epoch_ms",     &Config::timer_start_epoch_ms},
    {"_pomodoro_work_secs", &Config::timer_pomodoro_work_secs},
}};
constexpr std::array<TimerSlotBool, 3> kTimerBool = {{
    {"_running",  &Config::timer_running},
    {"_notified", &Config::timer_notified},
    {"_pomodoro", &Config::timer_pomodoro},
}};

// Per-alarm int fields with clamp range.
struct AlarmIntField { std::string_view suffix; int Alarm::*field; int lo, hi; };
constexpr std::array<AlarmIntField, 6> kAlarmInts = {{
    {"_days",   &Alarm::days_mask,  0, ALARM_ALL_DAYS},
    {"_hour",   &Alarm::hour,       0, 23},
    {"_min",    &Alarm::minute,     0, 59},
    {"_year",   &Alarm::date_year,  2000, 9999},
    {"_month",  &Alarm::date_month, 1, 12},
    {"_day",    &Alarm::date_day,   1, 31},
}};

// Parse "prefix<int><suffix>" (e.g. "alarm3_hour"). Returns idx=-1 if no integer follows the prefix.
struct IndexedKey { int idx; std::string_view suffix; };
inline IndexedKey parse_indexed(std::string_view key, std::string_view prefix) {
    if (!key.starts_with(prefix)) return {-1, {}};
    auto rest = key.substr(prefix.size());
    int i = -1;
    auto [ptr, ec] = std::from_chars(rest.data(), rest.data() + rest.size(), i);
    if (ec != std::errc{}) return {-1, {}};
    return {i, std::string_view{ptr, rest.data() + rest.size()}};
}

} // namespace

bool config_write(const Config& c, std::ostream& f) {
    const char* theme_str = c.theme_mode == ThemeMode::Dark    ? "dark"
                            : c.theme_mode == ThemeMode::Light ? "light"
                                                               : "auto";
    f << std::format("show_clk={}\nshow_sw={}\nshow_tmr={}\ntopmost={}\nsound_on_expiry={}\ntheme={}\nclock_view={}\nnum_timers={}\n",
                     c.show_clk ? 1 : 0, c.show_sw ? 1 : 0, c.show_tmr ? 1 : 0,
                     c.topmost ? 1 : 0, c.sound_on_expiry ? 1 : 0,
                     theme_str, (int)c.clock_view, c.num_timers);
    if (c.show_alarms) f << "show_alarms=1\n";
    if (!c.alarms.empty()) {
        f << std::format("num_alarms={}\n", (int)c.alarms.size());
        for (int i = 0; i < (int)c.alarms.size(); ++i) {
            const auto& a = c.alarms[i];
            if (!a.name.empty()) f << std::format("alarm{}_name={}\n", i, a.name);
            f << std::format("alarm{}_schedule={}\nalarm{}_hour={}\nalarm{}_min={}\n",
                             i, (int)a.schedule, i, a.hour, i, a.minute);
            if (a.schedule == AlarmSchedule::Days)
                f << std::format("alarm{}_days={}\n", i, a.days_mask);
            else
                f << std::format("alarm{}_year={}\nalarm{}_month={}\nalarm{}_day={}\n",
                                 i, a.date_year, i, a.date_month, i, a.date_day);
            if (!a.enabled) f << std::format("alarm{}_enabled=0\n", i);
        }
    }
    Config def;
    if (c.pomodoro_work_secs  != def.pomodoro_work_secs ||
        c.pomodoro_short_secs != def.pomodoro_short_secs ||
        c.pomodoro_long_secs  != def.pomodoro_long_secs)
        f << std::format("pomodoro_work={}\npomodoro_short={}\npomodoro_long={}\n",
                         c.pomodoro_work_secs, c.pomodoro_short_secs, c.pomodoro_long_secs);
    if (c.pomodoro_cadence    != def.pomodoro_cadence)    f << std::format("pomodoro_cadence={}\n", c.pomodoro_cadence);
    if (c.pomodoro_auto_start != def.pomodoro_auto_start) f << std::format("pomodoro_auto_start={}\n", c.pomodoro_auto_start ? 1 : 0);

    const AnalogClockStyle& a = c.analog_style;
    const AnalogClockStyle adef;
    for (const auto& fld : kAnalogInts)
        if (a.*fld.field != adef.*fld.field)
            f << std::format("{}={}\n", fld.key, a.*fld.field);
    if (a.show_minute_ticks != adef.show_minute_ticks) f << std::format("analog_show_min_ticks={}\n", a.show_minute_ticks ? 1 : 0);
    if (a.hour_labels       != adef.hour_labels)       f << std::format("analog_hour_labels={}\n", (int)a.hour_labels);

    if (c.num_custom_presets > 0) {
        f << std::format("num_custom_presets={}\n", c.num_custom_presets);
        for (int i = 0; i < c.num_custom_presets; ++i)
            f << std::format("custom_preset{}={}\n", i, c.custom_preset_secs[i]);
    }
    for (int i = 0; i < c.num_timers; ++i) {
        f << std::format("timer{}={}\n", i, c.timer_secs[i]);
        if (!c.timer_labels[i].empty()) f << std::format("timer{}_label={}\n", i, c.timer_labels[i]);
    }
    if (c.pos_valid) f << std::format("win_x={}\nwin_y={}\nwin_w={}\n", c.win_x, c.win_y, c.win_w);
    if (c.sw_elapsed_ms > 0 || c.sw_running) {
        f << std::format("sw_elapsed_ms={}\n", c.sw_elapsed_ms);
        f << std::format("sw_running={}\n", c.sw_running ? 1 : 0);
        if (c.sw_running && c.sw_start_epoch_ms > 0)
            f << std::format("sw_start_epoch_ms={}\n", c.sw_start_epoch_ms);
        if (!c.sw_lap_file.empty()) f << "sw_lap_file=" << c.sw_lap_file << "\n";
    }
    for (int i = 0; i < c.num_timers; ++i) {
        if (c.timer_elapsed_ms[i] > 0 || c.timer_running[i] || c.timer_notified[i]) {
            f << std::format("timer{}_elapsed_ms={}\n", i, c.timer_elapsed_ms[i]);
            if (c.timer_running[i]) {
                f << std::format("timer{}_running=1\n", i);
                if (c.timer_start_epoch_ms[i] > 0)
                    f << std::format("timer{}_start_epoch_ms={}\n", i, c.timer_start_epoch_ms[i]);
            }
            if (c.timer_notified[i]) f << std::format("timer{}_notified=1\n", i);
        }
        if (c.timer_pomodoro[i]) {
            f << std::format("timer{}_pomodoro=1\n", i);
            f << std::format("timer{}_pomodoro_phase={}\n", i, c.timer_pomodoro_phase[i]);
            if (c.timer_pomodoro_work_secs[i] > 0)
                f << std::format("timer{}_pomodoro_work_secs={}\n", i, c.timer_pomodoro_work_secs[i]);
        }
    }
    return f.good();
}

namespace {

// Try to read a string-valued key. Returns true if handled (caller skips numeric parse).
bool read_string_key(Config& c, std::string_view key, std::string_view rest) {
    if (key == "theme") {
        c.theme_mode = rest == "dark"    ? ThemeMode::Dark
                       : rest == "light" ? ThemeMode::Light
                                         : ThemeMode::Auto;
        return true;
    }
    if (key == "sw_lap_file") { c.sw_lap_file = std::string(rest); return true; }
    if (auto k = parse_indexed(key, "timer"); k.idx >= 0 && k.suffix == "_label" && k.idx < Config::MAX_TIMERS) {
        c.timer_labels[k.idx] = std::string(rest.substr(0, Config::MAX_LABEL_LEN));
        return true;
    }
    if (auto k = parse_indexed(key, "alarm"); k.idx >= 0 && k.suffix == "_name" && k.idx < ALARM_MAX_COUNT) {
        if (k.idx >= (int)c.alarms.size()) c.alarms.resize(k.idx + 1);
        c.alarms[k.idx].name = std::string(rest.substr(0, 40));
        return true;
    }
    return false;
}

bool read_analog_key(AnalogClockStyle& a, std::string_view key, long long val) {
    for (const auto& fld : kAnalogInts) {
        if (key == fld.key) {
            a.*fld.field = (fld.lo == NO_CLAMP_LO && fld.hi == NO_CLAMP_HI) ? (int)val : clamp_int(val, fld.lo, fld.hi);
            return true;
        }
    }
    if (key == "analog_show_min_ticks") { a.show_minute_ticks = val != 0; return true; }
    if (key == "analog_hour_labels")    { a.hour_labels = (HourLabels)clamp_int(val, 0, 2); return true; }
    return false;
}

bool read_alarm_key(Config& c, std::string_view key, long long val) {
    auto k = parse_indexed(key, "alarm");
    if (k.idx < 0 || k.idx >= ALARM_MAX_COUNT) return false;
    if (k.idx >= (int)c.alarms.size()) c.alarms.resize(k.idx + 1);
    Alarm& a = c.alarms[k.idx];
    if (k.suffix == "_schedule") { a.schedule = (AlarmSchedule)clamp_int(val, 0, 1); return true; }
    if (k.suffix == "_enabled")  { a.enabled = val != 0; return true; }
    for (const auto& fld : kAlarmInts)
        if (k.suffix == fld.suffix) { a.*fld.field = clamp_int(val, fld.lo, fld.hi); return true; }
    return false;
}

bool read_timer_key(Config& c, std::string_view key, long long val) {
    auto k = parse_indexed(key, "timer");
    if (k.idx < 0 || k.idx >= Config::MAX_TIMERS) return false;
    if (k.suffix.empty()) {
        c.timer_secs[k.idx] = clamp_int(val, Config::TIMER_MIN_SECS, Config::TIMER_MAX_SECS);
        return true;
    }
    if (k.suffix == "_pomodoro_phase") {
        c.timer_pomodoro_phase[k.idx] = clamp_int(val, 0, pomodoro_phase_count(POMODORO_MAX_CADENCE) - 1);
        return true;
    }
    for (const auto& fld : kTimerI64)
        if (k.suffix == fld.suffix) { (c.*fld.field)[k.idx] = std::max(val, 0LL); return true; }
    for (const auto& fld : kTimerBool)
        if (k.suffix == fld.suffix) { (c.*fld.field)[k.idx] = val != 0; return true; }
    return false;
}

void normalize_alarm_dates(std::vector<Alarm>& alarms) {
    for (auto& a : alarms) {
        if (a.schedule != AlarmSchedule::Date) continue;
        int max_day = 31;
        int mo = a.date_month, yr = a.date_year;
        if (mo == 4 || mo == 6 || mo == 9 || mo == 11) max_day = 30;
        else if (mo == 2) max_day = (yr % 4 == 0 && (yr % 100 != 0 || yr % 400 == 0)) ? 29 : 28;
        if (a.date_day > max_day) a.date_day = max_day;
    }
}

} // namespace

bool config_read(Config& c, std::istream& f) {
    bool has_x = false, has_y = false, has_w = false;
    for (std::string line; std::getline(f, line);) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string_view key{line.data(), eq};
        std::string_view rest{line.data() + eq + 1, line.size() - eq - 1};

        if (read_string_key(c, key, rest)) continue;
        long long val;
        if (std::from_chars(rest.data(), rest.data() + rest.size(), val).ec != std::errc{}) continue;

        if (read_analog_key(c.analog_style, key, val)) continue;
        if (read_timer_key(c, key, val)) continue; // also handles bare "timer<i>="
        if (read_alarm_key(c, key, val)) continue;

        if      (key == "show_clk")            c.show_clk = val != 0;
        else if (key == "show_sw")             c.show_sw = val != 0;
        else if (key == "show_tmr")            c.show_tmr = val != 0;
        else if (key == "show_alarms")         c.show_alarms = val != 0;
        else if (key == "topmost")             c.topmost = val != 0;
        else if (key == "sound_on_expiry")     c.sound_on_expiry = val != 0;
        else if (key == "num_timers")          c.num_timers = clamp_int(val, 1, Config::MAX_TIMERS);
        else if (key == "num_alarms") {
            int n = clamp_int(val, 0, ALARM_MAX_COUNT);
            if (n > (int)c.alarms.size()) c.alarms.resize(n);
        }
        else if (key == "clock_view")          c.clock_view = (ClockView)clamp_int(val, 0, CLOCK_VIEW_COUNT - 1);
        else if (key == "win_x")               { c.win_x = clamp_int(val, INT_MIN, INT_MAX); has_x = true; }
        else if (key == "win_y")               { c.win_y = clamp_int(val, INT_MIN, INT_MAX); has_y = true; }
        else if (key == "win_w")               { c.win_w = clamp_int(val, Config::MIN_WINDOW_W, INT_MAX); has_w = true; }
        else if (key == "pomodoro_work")       c.pomodoro_work_secs  = clamp_int(val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_short")      c.pomodoro_short_secs = clamp_int(val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_long")       c.pomodoro_long_secs  = clamp_int(val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_cadence")    c.pomodoro_cadence    = clamp_int(val, POMODORO_MIN_CADENCE, POMODORO_MAX_CADENCE);
        else if (key == "pomodoro_auto_start") c.pomodoro_auto_start = val != 0;
        else if (key == "sw_running")          c.sw_running = val != 0;
        else if (key == "sw_elapsed_ms")       c.sw_elapsed_ms = std::max(val, 0LL);
        else if (key == "sw_start_epoch_ms")   c.sw_start_epoch_ms = std::max(val, 0LL);
        else if (key == "num_custom_presets")  c.num_custom_presets = clamp_int(val, 0, Config::MAX_CUSTOM_PRESETS);
        else if (auto k = parse_indexed(key, "custom_preset");
                 k.idx >= 0 && k.suffix.empty() && k.idx < Config::MAX_CUSTOM_PRESETS)
            c.custom_preset_secs[k.idx] = clamp_int(val, 1, Config::TIMER_MAX_SECS);
    }
    c.pos_valid = has_x && has_y && has_w;
    normalize_alarm_dates(c.alarms);
    return true;
}
