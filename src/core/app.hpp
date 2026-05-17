#pragma once
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>
#include "alarm.hpp"
#include "config.hpp"
#include "stopwatch.hpp"
#include "timer.hpp"

struct TimerSlot {
    Timer t;
    std::chrono::seconds dur{std::chrono::seconds{60}};
    bool notified = false;
    std::wstring label;
    bool pomodoro = false;
    int pomodoro_phase = 0;
    std::chrono::seconds pomodoro_work_elapsed{};
};

inline void advance_pomodoro_phase(TimerSlot& ts, int work_secs, int short_secs, int long_secs,
                                    int cadence, bool auto_start,
                                    std::chrono::steady_clock::time_point now) {
    if (pomodoro_is_work(ts.pomodoro_phase))
        ts.pomodoro_work_elapsed += ts.dur;
    ts.pomodoro_phase = pomodoro_next_phase(ts.pomodoro_phase, cadence);
    auto secs = std::chrono::seconds{pomodoro_phase_secs(ts.pomodoro_phase, work_secs, short_secs, long_secs, cadence)};
    ts.dur = secs;
    ts.t.reset();
    ts.t.set(secs);
    if (auto_start) ts.t.start(now);
    ts.notified = false;
    ts.label = pomodoro_phase_label(ts.pomodoro_phase, cadence);
}

struct App {
    Stopwatch sw;
    std::filesystem::path sw_lap_file;
    std::vector<TimerSlot> timers;
    bool show_clk = true;
    bool show_sw = true;
    bool show_tmr = true;
    bool topmost = false;
    bool sound_on_expiry = true;
    ThemeMode theme_mode = ThemeMode::Auto;
    ClockView clock_view = ClockView::H24_HMS;
    AnalogClockStyle analog_style;
    int pomodoro_work_secs = 25 * 60;
    int pomodoro_short_secs = 5 * 60;
    int pomodoro_long_secs = 15 * 60;
    int pomodoro_cadence = POMODORO_DEFAULT_CADENCE;
    bool pomodoro_auto_start = true;
    std::vector<int> custom_preset_secs;
    bool show_alarms = false;
    std::vector<Alarm> alarms;
    int alarm_notified_minute = -1;    // wall-clock minute (0–1439) when notified flags were last reset
    bool show_help = false;
    bool lap_write_failed = false;
    int blink_act = 0;
    std::chrono::steady_clock::time_point blink_t{};
    App() {
        timers.resize(1);
        for (auto& ts : timers) ts.t.set(ts.dur);
    }
};
