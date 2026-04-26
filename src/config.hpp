#pragma once
#include <array>
#include <string>

#include "pomodoro.hpp"

enum class ThemeMode { Auto = 0, Dark = 1, Light = 2 };
enum class ClockView { H24_HMS = 0, H24_HM = 1, H12_HMS = 2, H12_HM = 3 };
inline constexpr int CLOCK_VIEW_COUNT = 4;
static_assert((int)ClockView::H12_HM == CLOCK_VIEW_COUNT - 1,
              "CLOCK_VIEW_COUNT out of sync with ClockView enum");

struct Config {
    bool show_clk = true;
    bool show_sw = true;
    bool show_tmr = true;
    bool topmost = false;
    ThemeMode theme_mode = ThemeMode::Auto;
    static constexpr int MAX_TIMERS = 20;
    static constexpr int MAX_LABEL_LEN = 20;
    static constexpr int TIMER_MIN_SECS = 0;
    static constexpr int TIMER_MAX_SECS = 86400;
    static constexpr int MIN_WINDOW_W = 260;
    int num_timers = 1;
    std::array<int, MAX_TIMERS> timer_secs{};
    std::array<std::string, MAX_TIMERS> timer_labels{};
    bool pos_valid = false;
    int win_x = 0, win_y = 0, win_w = 340;
    // Runtime state — stopwatch
    bool sw_running = false;
    long long sw_elapsed_ms = 0;
    long long sw_start_epoch_ms = 0;
    std::string sw_lap_file;
    // Runtime state — timers
    std::array<bool, MAX_TIMERS> timer_running{};
    std::array<long long, MAX_TIMERS> timer_elapsed_ms{};
    std::array<long long, MAX_TIMERS> timer_start_epoch_ms{};
    std::array<bool, MAX_TIMERS> timer_notified{};
    std::array<bool, MAX_TIMERS> timer_pomodoro{};
    std::array<int, MAX_TIMERS> timer_pomodoro_phase{};
    std::array<long long, MAX_TIMERS> timer_pomodoro_work_secs{};
    int pomodoro_work_secs = POMODORO_WORK_SECS;
    int pomodoro_short_secs = POMODORO_SHORT_BREAK_SECS;
    int pomodoro_long_secs = POMODORO_LONG_BREAK_SECS;
    ClockView clock_view = ClockView::H24_HMS;
    Config() { timer_secs.fill(60); }
};
