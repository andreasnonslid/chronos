#pragma once
#include <algorithm>
#include <charconv>
#include <climits>
#include <format>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include "config.hpp"

inline bool config_write(const Config& c, std::ostream& f) {
    const char* theme_str = c.theme_mode == ThemeMode::Dark ? "dark" : c.theme_mode == ThemeMode::Light ? "light" : "auto";
    f << std::format("show_clk={}\nshow_sw={}\nshow_tmr={}\ntopmost={}\nsound_on_expiry={}\ntheme={}\nclock_view={}\nnum_timers={}\n", c.show_clk ? 1 : 0,
                     c.show_sw ? 1 : 0, c.show_tmr ? 1 : 0, c.topmost ? 1 : 0, c.sound_on_expiry ? 1 : 0, theme_str, (int)c.clock_view, c.num_timers);
    Config defaults;
    if (c.pomodoro_work_secs != defaults.pomodoro_work_secs ||
        c.pomodoro_short_secs != defaults.pomodoro_short_secs ||
        c.pomodoro_long_secs != defaults.pomodoro_long_secs)
        f << std::format("pomodoro_work={}\npomodoro_short={}\npomodoro_long={}\n",
                         c.pomodoro_work_secs, c.pomodoro_short_secs, c.pomodoro_long_secs);
    if (c.pomodoro_cadence != defaults.pomodoro_cadence)
        f << std::format("pomodoro_cadence={}\n", c.pomodoro_cadence);
    if (c.pomodoro_auto_start != defaults.pomodoro_auto_start)
        f << std::format("pomodoro_auto_start={}\n", c.pomodoro_auto_start ? 1 : 0);
    {
        const auto& a = c.analog_style;
        AnalogClockStyle def;
        if (a.hour_color != def.hour_color) f << std::format("analog_hour_color={}\n", a.hour_color);
        if (a.minute_color != def.minute_color) f << std::format("analog_minute_color={}\n", a.minute_color);
        if (a.second_color != def.second_color) f << std::format("analog_second_color={}\n", a.second_color);
        if (a.face_color != def.face_color) f << std::format("analog_face_color={}\n", a.face_color);
        if (a.tick_color != def.tick_color) f << std::format("analog_tick_color={}\n", a.tick_color);
        if (a.hour_len_pct != def.hour_len_pct) f << std::format("analog_hour_len={}\n", a.hour_len_pct);
        if (a.minute_len_pct != def.minute_len_pct) f << std::format("analog_minute_len={}\n", a.minute_len_pct);
        if (a.second_len_pct != def.second_len_pct) f << std::format("analog_second_len={}\n", a.second_len_pct);
        if (a.hour_thickness != def.hour_thickness) f << std::format("analog_hour_thick={}\n", a.hour_thickness);
        if (a.minute_thickness != def.minute_thickness) f << std::format("analog_minute_thick={}\n", a.minute_thickness);
        if (a.second_thickness != def.second_thickness) f << std::format("analog_second_thick={}\n", a.second_thickness);
        if (a.hour_tick_pct != def.hour_tick_pct) f << std::format("analog_hour_tick={}\n", a.hour_tick_pct);
        if (a.minute_tick_pct != def.minute_tick_pct) f << std::format("analog_minute_tick={}\n", a.minute_tick_pct);
        if (a.show_minute_ticks != def.show_minute_ticks) f << std::format("analog_show_min_ticks={}\n", a.show_minute_ticks ? 1 : 0);
        if (a.hour_labels != def.hour_labels) f << std::format("analog_hour_labels={}\n", (int)a.hour_labels);
    }
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
            if (c.timer_pomodoro_work_secs[i] > 0)
                f << std::format("timer{}_pomodoro_work_secs={}\n", i, c.timer_pomodoro_work_secs[i]);
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
        auto clamp_int = [](long long v, int lo, int hi) -> int {
            return (int)std::clamp(v, (long long)lo, (long long)hi);
        };
        if (key == "clock_view") {
            c.clock_view = (ClockView)clamp_int(val, 0, CLOCK_VIEW_COUNT - 1);
        } else if (key == "num_custom_presets") {
            c.num_custom_presets = clamp_int(val, 0, Config::MAX_CUSTOM_PRESETS);
        } else if (key.starts_with("custom_preset") && key.size() > 13) {
            auto idx_str = key.substr(13);
            int i;
            auto [ptr, ec2] = std::from_chars(idx_str.data(), idx_str.data() + idx_str.size(), i);
            if (ec2 == std::errc{} && ptr == idx_str.data() + idx_str.size() &&
                i >= 0 && i < Config::MAX_CUSTOM_PRESETS)
                c.custom_preset_secs[i] = clamp_int(val, 1, Config::TIMER_MAX_SECS);
        } else if (key == "show_clk")
            c.show_clk = val != 0;
        else if (key == "show_sw")
            c.show_sw = val != 0;
        else if (key == "show_tmr")
            c.show_tmr = val != 0;
        else if (key == "topmost")
            c.topmost = val != 0;
        else if (key == "sound_on_expiry")
            c.sound_on_expiry = val != 0;
        else if (key == "num_timers")
            c.num_timers = clamp_int(val, 1, Config::MAX_TIMERS);
        else if (key == "win_x") {
            c.win_x = clamp_int(val, INT_MIN, INT_MAX);
            has_x = true;
        } else if (key == "win_y") {
            c.win_y = clamp_int(val, INT_MIN, INT_MAX);
            has_y = true;
        } else if (key == "win_w") {
            c.win_w = clamp_int(val, Config::MIN_WINDOW_W, INT_MAX);
            has_w = true;
        } else if (key == "pomodoro_work")
            c.pomodoro_work_secs = clamp_int(val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_short")
            c.pomodoro_short_secs = clamp_int(val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_long")
            c.pomodoro_long_secs = clamp_int(val, 60, Config::TIMER_MAX_SECS);
        else if (key == "pomodoro_cadence")
            c.pomodoro_cadence = clamp_int(val, POMODORO_MIN_CADENCE, POMODORO_MAX_CADENCE);
        else if (key == "pomodoro_auto_start")
            c.pomodoro_auto_start = val != 0;
        else if (key == "analog_hour_color") c.analog_style.hour_color = (int)val;
        else if (key == "analog_minute_color") c.analog_style.minute_color = (int)val;
        else if (key == "analog_second_color") c.analog_style.second_color = (int)val;
        else if (key == "analog_face_color") c.analog_style.face_color = (int)val;
        else if (key == "analog_tick_color") c.analog_style.tick_color = (int)val;
        else if (key == "analog_hour_len") c.analog_style.hour_len_pct = clamp_int(val, 40, 80);
        else if (key == "analog_minute_len") c.analog_style.minute_len_pct = clamp_int(val, 60, 95);
        else if (key == "analog_second_len") c.analog_style.second_len_pct = clamp_int(val, 70, 98);
        else if (key == "analog_hour_thick") c.analog_style.hour_thickness = clamp_int(val, 1, 6);
        else if (key == "analog_minute_thick") c.analog_style.minute_thickness = clamp_int(val, 1, 4);
        else if (key == "analog_second_thick") c.analog_style.second_thickness = clamp_int(val, 1, 3);
        else if (key == "analog_hour_tick") c.analog_style.hour_tick_pct = clamp_int(val, 5, 20);
        else if (key == "analog_minute_tick") c.analog_style.minute_tick_pct = clamp_int(val, 2, 10);
        else if (key == "analog_show_min_ticks") c.analog_style.show_minute_ticks = val != 0;
        else if (key == "analog_hour_labels") c.analog_style.hour_labels = (HourLabels)clamp_int(val, 0, 2);
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
                    c.timer_secs[i] = clamp_int(val, Config::TIMER_MIN_SECS, Config::TIMER_MAX_SECS);
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
                    c.timer_pomodoro_phase[i] = clamp_int(val, 0, pomodoro_phase_count(POMODORO_MAX_CADENCE) - 1);
                else if (field == "_pomodoro_work_secs")
                    c.timer_pomodoro_work_secs[i] = std::max(val, 0LL);
            }
        }
    }
    c.pos_valid = has_x && has_y && has_w;
    return true;
}
