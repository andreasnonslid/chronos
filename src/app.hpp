#pragma once
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>
#include "config.hpp"
#include "stopwatch.hpp"
#include "timer.hpp"

struct TimerSlot {
    Timer t;
    std::chrono::seconds dur{std::chrono::seconds{60}};
    bool notified = false;
    std::wstring label;
    bool pomodoro = false;
    PomodoroPhase pomodoro_phase = PomodoroPhase::Work1;
    std::chrono::seconds pomodoro_work_elapsed{};
};

inline void advance_pomodoro_phase(TimerSlot& ts, int work_secs, int short_secs, int long_secs,
                                    std::chrono::steady_clock::time_point now) {
    if (pomodoro_is_work(ts.pomodoro_phase))
        ts.pomodoro_work_elapsed += ts.dur;
    ts.pomodoro_phase = pomodoro_next_phase(ts.pomodoro_phase);
    auto secs = std::chrono::seconds{pomodoro_phase_secs(ts.pomodoro_phase, work_secs, short_secs, long_secs)};
    ts.dur = secs;
    ts.t.reset();
    ts.t.set(secs);
    ts.t.start(now);
    ts.notified = false;
    ts.label = pomodoro_phase_label(ts.pomodoro_phase);
}

struct App {
    Stopwatch sw;
    std::filesystem::path sw_lap_file;
    std::vector<TimerSlot> timers;
    bool show_clk = true;
    bool show_sw = true;
    bool show_tmr = true;
    bool topmost = false;
    ThemeMode theme_mode = ThemeMode::Auto;
    ClockView clock_view = ClockView::H24_HMS;
    int pomodoro_work_secs = 25 * 60;
    int pomodoro_short_secs = 5 * 60;
    int pomodoro_long_secs = 15 * 60;
    bool show_help = false;
    bool lap_write_failed = false;
    int blink_act = 0;
    std::chrono::steady_clock::time_point blink_t{};
    App() {
        timers.resize(1);
        for (auto& ts : timers) ts.t.set(ts.dur);
    }
};
