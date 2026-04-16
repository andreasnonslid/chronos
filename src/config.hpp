#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include "pomodoro.hpp"

enum class ThemeMode { Auto = 0, Dark = 1, Light = 2 };

struct Config {
    bool show_clk = true;
    bool show_sw = true;
    bool show_tmr = true;
    bool topmost = false;
    ThemeMode theme_mode = ThemeMode::Auto;
    static constexpr int MAX_TIMERS = 3;
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
    int pomodoro_work_secs = POMODORO_WORK_SECS;
    int pomodoro_short_secs = POMODORO_SHORT_BREAK_SECS;
    int pomodoro_long_secs = POMODORO_LONG_BREAK_SECS;
    Config() { timer_secs.fill(60); }
};

inline bool config_write(const Config& c, std::ostream& f) {
    const char* theme_str = c.theme_mode == ThemeMode::Dark ? "dark" : c.theme_mode == ThemeMode::Light ? "light" : "auto";
    f << std::format("show_clk={}\nshow_sw={}\nshow_tmr={}\ntopmost={}\ntheme={}\nnum_timers={}\n", c.show_clk ? 1 : 0,
                     c.show_sw ? 1 : 0, c.show_tmr ? 1 : 0, c.topmost ? 1 : 0, theme_str, c.num_timers);
    Config defaults;
    if (c.pomodoro_work_secs != defaults.pomodoro_work_secs ||
        c.pomodoro_short_secs != defaults.pomodoro_short_secs ||
        c.pomodoro_long_secs != defaults.pomodoro_long_secs)
        f << std::format("pomodoro_work={}\npomodoro_short={}\npomodoro_long={}\n",
                         c.pomodoro_work_secs, c.pomodoro_short_secs, c.pomodoro_long_secs);
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
        if (!c.sw_lap_file.empty())
            f << "sw_lap_file=" << c.sw_lap_file << "\n";
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
        }
    }
    return f.good();
}

inline bool config_read(Config& c, std::istream& f) {
    bool has_x = false, has_y = false, has_w = false;
    for (std::string line; std::getline(f, line);) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string_view key{line.data(), eq};
        std::string_view rest{line.data() + eq + 1, line.size() - eq - 1};
        // Handle string-valued keys before numeric parsing
        bool handled_str = false;
        if (key == "theme") {
            if (rest == "dark") c.theme_mode = ThemeMode::Dark;
            else if (rest == "light") c.theme_mode = ThemeMode::Light;
            else c.theme_mode = ThemeMode::Auto;
            handled_str = true;
        } else if (key == "sw_lap_file") {
            c.sw_lap_file = std::string(rest);
            handled_str = true;
        } else if (key.starts_with("timer") && key.ends_with("_label")) {
            auto num = key.substr(5, key.size() - 11);
            int i;
            auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), i);
            if (ec == std::errc{} && ptr == num.data() + num.size() && i >= 0 && i < Config::MAX_TIMERS) {
                c.timer_labels[i] = std::string(rest.substr(0, Config::MAX_LABEL_LEN));
                handled_str = true;
            }
        }
        if (handled_str) continue;
        long long val;
        if (std::from_chars(rest.data(), rest.data() + rest.size(), val).ec != std::errc{}) continue;
        if (key == "show_clk")
            c.show_clk = val != 0;
        else if (key == "show_sw")
            c.show_sw = val != 0;
        else if (key == "show_tmr")
            c.show_tmr = val != 0;
        else if (key == "topmost")
            c.topmost = val != 0;
        else if (key == "num_timers")
            c.num_timers = std::clamp((int)val, 1, Config::MAX_TIMERS);
        else if (key == "win_x") {
            c.win_x = (int)val;
            has_x = true;
        } else if (key == "win_y") {
            c.win_y = (int)val;
            has_y = true;
        } else if (key == "win_w") {
            c.win_w = std::max(Config::MIN_WINDOW_W, (int)val);
            has_w = true;
        } else if (key == "pomodoro_work")
            c.pomodoro_work_secs = std::clamp((int)val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_short")
            c.pomodoro_short_secs = std::clamp((int)val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_long")
            c.pomodoro_long_secs = std::clamp((int)val, 60, Config::TIMER_MAX_SECS);
        else if (key == "sw_running")
            c.sw_running = val != 0;
        else if (key == "sw_elapsed_ms")
            c.sw_elapsed_ms = std::max(val, 0LL);
        else if (key == "sw_start_epoch_ms")
            c.sw_start_epoch_ms = std::max(val, 0LL);
        else if (key.starts_with("timer") && key.size() > 5) {
            auto suffix = key.substr(5);
            int i = -1;
            auto [ptr, ec] = std::from_chars(suffix.data(), suffix.data() + suffix.size(), i);
            if (ec != std::errc{} || i < 0 || i >= Config::MAX_TIMERS) { /* skip */ }
            else {
                std::string_view field{ptr, suffix.data() + suffix.size()};
                if (field.empty())
                    c.timer_secs[i] = std::clamp((int)val, Config::TIMER_MIN_SECS, Config::TIMER_MAX_SECS);
                else if (field == "_elapsed_ms")
                    c.timer_elapsed_ms[i] = std::max(val, 0LL);
                else if (field == "_running")
                    c.timer_running[i] = val != 0;
                else if (field == "_start_epoch_ms")
                    c.timer_start_epoch_ms[i] = std::max(val, 0LL);
                else if (field == "_notified")
                    c.timer_notified[i] = val != 0;
                else if (field == "_pomodoro")
                    c.timer_pomodoro[i] = val != 0;
                else if (field == "_pomodoro_phase")
                    c.timer_pomodoro_phase[i] = (int)val % 8;
            }
        }
    }
    c.pos_valid = has_x && has_y && has_w;
    return true;
}
