#include "config_io.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include "config.hpp"
#include "config_serial.hpp"
#include "encoding.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <cstdlib>
#endif

std::filesystem::path config_path() {
#ifdef _WIN32
    wchar_t appdata[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) {
        auto dir = std::filesystem::path{appdata} / L"Chronos";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (!ec) return dir / L"config.ini";
    }
    return std::filesystem::path{L"config.ini"};
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    std::filesystem::path dir;
    if (xdg && xdg[0] != '\0') {
        dir = std::filesystem::path{xdg} / "Chronos";
    } else {
        const char* home = std::getenv("HOME");
        dir = std::filesystem::path{home ? home : "."} / ".config" / "Chronos";
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (!ec) return dir / "config.ini";
    return std::filesystem::path{"config.ini"};
#endif
}

void save_config(App& app, const std::filesystem::path& path) {
    using namespace std::chrono;
    auto tmp = path;
    tmp += ".tmp";
    std::ofstream f(tmp);
    if (!f) return;

    Config cfg;
    cfg.show_clk          = app.show_clk;
    cfg.clock_view        = app.clock_view;
    cfg.clock_split_mode  = app.clock_split_mode;
    cfg.clock_split_pct   = app.clock_split_pct;
    cfg.show_sw           = app.show_sw;
    cfg.show_tmr          = app.show_tmr;
    cfg.show_alarms       = app.show_alarms;
    cfg.alarms            = app.alarms;
    cfg.topmost           = app.topmost;
    cfg.sound_on_expiry   = app.sound_on_expiry;
    cfg.theme_mode        = app.theme_mode;
    cfg.analog_style      = app.analog_style;
    cfg.pomodoro_work_secs  = app.pomodoro_work_secs;
    cfg.pomodoro_short_secs = app.pomodoro_short_secs;
    cfg.pomodoro_long_secs  = app.pomodoro_long_secs;
    cfg.pomodoro_cadence    = app.pomodoro_cadence;
    cfg.pomodoro_auto_start = app.pomodoro_auto_start;
    cfg.num_custom_presets  = (int)app.custom_preset_secs.size();
    for (int i = 0; i < cfg.num_custom_presets; ++i)
        cfg.custom_preset_secs[i] = app.custom_preset_secs[i];
    cfg.num_timers = (int)app.timers.size();

    auto now_steady  = steady_clock::now();
    auto now_wall_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    cfg.sw_running = app.sw.is_running();
    cfg.sw_elapsed_ms = duration_cast<milliseconds>(app.sw.elapsed(now_steady)).count();
    if (cfg.sw_running) cfg.sw_start_epoch_ms = now_wall_ms;
    if (!app.sw_lap_file.empty()) cfg.sw_lap_file = wide_to_utf8(app.sw_lap_file.wstring());

    for (int i = 0; i < cfg.num_timers; ++i) {
        const auto& ts = app.timers[i];
        cfg.timer_secs[i]     = (int)duration_cast<seconds>(ts.dur).count();
        cfg.timer_running[i]  = ts.t.is_running();
        cfg.timer_notified[i] = ts.notified;
        if (ts.t.touched()) {
            cfg.timer_elapsed_ms[i] = duration_cast<milliseconds>(
                ts.dur - ts.t.remaining(now_steady)).count();
            if (cfg.timer_running[i])
                cfg.timer_start_epoch_ms[i] = now_wall_ms;
        }
        cfg.timer_labels[i]                  = wide_to_utf8(ts.label);
        cfg.timer_pomodoro[i]                = ts.pomodoro;
        cfg.timer_pomodoro_phase[i]          = ts.pomodoro_phase;
        cfg.timer_pomodoro_work_secs[i]      =
            (long long)duration_cast<seconds>(ts.pomodoro_work_elapsed).count();
    }

    config_write(cfg, f);
    f.close();
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
}

bool load_config(App& app, const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) return false;

    Config cfg;
    if (!config_read(cfg, f)) return false;

    app.show_clk          = cfg.show_clk;
    app.clock_view        = cfg.clock_view;
    app.clock_split_mode  = cfg.clock_split_mode;
    app.clock_split_pct   = cfg.clock_split_pct;
    app.show_sw           = cfg.show_sw;
    app.show_tmr          = cfg.show_tmr;
    app.show_alarms       = cfg.show_alarms;
    app.alarms            = cfg.alarms;
    app.topmost           = cfg.topmost;
    app.sound_on_expiry   = cfg.sound_on_expiry;
    app.theme_mode        = cfg.theme_mode;
    app.analog_style      = cfg.analog_style;
    app.pomodoro_work_secs  = cfg.pomodoro_work_secs;
    app.pomodoro_short_secs = cfg.pomodoro_short_secs;
    app.pomodoro_long_secs  = cfg.pomodoro_long_secs;
    app.pomodoro_cadence    = cfg.pomodoro_cadence;
    app.pomodoro_auto_start = cfg.pomodoro_auto_start;
    if (cfg.num_custom_presets > 0) {
        app.custom_preset_secs.resize(cfg.num_custom_presets);
        for (int i = 0; i < cfg.num_custom_presets; ++i)
            app.custom_preset_secs[i] = cfg.custom_preset_secs[i];
    }

    using namespace std::chrono;
    int nt = std::max(1, std::min(cfg.num_timers, Config::MAX_TIMERS));
    app.timers.resize(nt);
    auto now_steady  = steady_clock::now();
    auto now_wall_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    if (cfg.sw_elapsed_ms > 0 || cfg.sw_running) {
        long long actual_ms = cfg.sw_elapsed_ms;
        if (cfg.sw_running && cfg.sw_start_epoch_ms > 0) {
            long long delta = now_wall_ms - cfg.sw_start_epoch_ms;
            if (delta > 0) actual_ms += delta;
        }
        app.sw.restore(milliseconds{std::max(actual_ms, 0LL)}, cfg.sw_running, now_steady);
        if (!cfg.sw_lap_file.empty()) {
            auto p = std::filesystem::path{utf8_to_wide(cfg.sw_lap_file)};
            std::error_code ec;
            if (std::filesystem::exists(p, ec)) app.sw_lap_file = std::move(p);
        }
    }

    for (int i = 0; i < nt; ++i) {
        auto& ts = app.timers[i];
        ts.dur    = seconds{std::max(0, cfg.timer_secs[i])};
        ts.t.set(ts.dur);
        ts.notified             = cfg.timer_notified[i];
        ts.label                = utf8_to_wide(cfg.timer_labels[i]);
        ts.pomodoro             = cfg.timer_pomodoro[i];
        ts.pomodoro_phase       = cfg.timer_pomodoro_phase[i];
        ts.pomodoro_work_elapsed= seconds{cfg.timer_pomodoro_work_secs[i]};

        if (cfg.timer_elapsed_ms[i] > 0 || cfg.timer_running[i]) {
            long long elapsed_ms = cfg.timer_elapsed_ms[i];
            if (cfg.timer_running[i] && cfg.timer_start_epoch_ms[i] > 0) {
                long long delta = now_wall_ms - cfg.timer_start_epoch_ms[i];
                if (delta > 0) elapsed_ms += delta;
            }
            long long dur_ms = (long long)duration_cast<milliseconds>(ts.dur).count();
            elapsed_ms = std::clamp(elapsed_ms, 0LL, dur_ms);
            bool actually_running = cfg.timer_running[i] && elapsed_ms < dur_ms;
            ts.t.restore(ts.dur, milliseconds{elapsed_ms}, actually_running, now_steady);
        }
    }
    return true;
}
