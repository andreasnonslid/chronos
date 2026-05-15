#include "config_io.hpp"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include "config_serial.hpp"
#include "debug.hpp"
#include "encoding.hpp"
#include "layout.hpp"
#include "wndstate.hpp"

std::filesystem::path config_path() {
    if (auto* appdata = _wgetenv(L"APPDATA")) {
        auto dir = std::filesystem::path{appdata} / L"Chronos";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (!ec) return dir / L"config.ini";
    }
    wchar_t exe[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exe, MAX_PATH))
        return std::filesystem::path{exe}.parent_path() / "config.ini";
    return std::filesystem::path{L"config.ini"};
}

void save_config(HWND hwnd, const WndState& s) {
    using namespace std::chrono;
    const auto& path = s.cfg_path;
    dbg(std::format(L"[chrono] save_config: {}", path.wstring()));
    auto tmp = path;
    tmp += L".tmp";
    std::ofstream f(tmp);
    if (!f) {
        dbg(L"[chrono] save_config: open failed");
        return;
    }
    Config cfg;
    cfg.show_clk = s.app.show_clk;
    cfg.clock_view = s.app.clock_view;
    cfg.show_sw = s.app.show_sw;
    cfg.show_tmr = s.app.show_tmr;
    cfg.show_alarms = s.app.show_alarms;
    cfg.alarms = s.app.alarms;
    cfg.topmost = s.app.topmost;
    cfg.sound_on_expiry = s.app.sound_on_expiry;
    cfg.theme_mode = s.app.theme_mode;
    cfg.analog_style = s.app.analog_style;
    cfg.pomodoro_work_secs = s.app.pomodoro_work_secs;
    cfg.pomodoro_short_secs = s.app.pomodoro_short_secs;
    cfg.pomodoro_long_secs = s.app.pomodoro_long_secs;
    cfg.pomodoro_cadence = s.app.pomodoro_cadence;
    cfg.pomodoro_auto_start = s.app.pomodoro_auto_start;
    cfg.num_custom_presets = (int)s.app.custom_preset_secs.size();
    for (int i = 0; i < cfg.num_custom_presets; ++i)
        cfg.custom_preset_secs[i] = s.app.custom_preset_secs[i];
    cfg.num_timers = (int)s.app.timers.size();
    auto now_steady = steady_clock::now();
    auto now_wall_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cfg.sw_running = s.app.sw.is_running();
    cfg.sw_elapsed_ms = duration_cast<milliseconds>(s.app.sw.elapsed(now_steady)).count();
    if (cfg.sw_running) cfg.sw_start_epoch_ms = now_wall_ms;
    if (!s.app.sw_lap_file.empty())
        cfg.sw_lap_file = wide_to_utf8(s.app.sw_lap_file.wstring());
    for (int i = 0; i < (int)s.app.timers.size(); ++i) {
        const auto& ts = s.app.timers[i];
        cfg.timer_secs[i] = (int)ts.dur.count();
        cfg.timer_labels[i] = wide_to_utf8(ts.label);
        cfg.timer_running[i] = ts.t.is_running();
        cfg.timer_elapsed_ms[i] = duration_cast<milliseconds>(ts.t.total_elapsed(now_steady)).count();
        if (cfg.timer_running[i]) cfg.timer_start_epoch_ms[i] = now_wall_ms;
        cfg.timer_notified[i] = ts.notified;
        cfg.timer_pomodoro[i] = ts.pomodoro;
        cfg.timer_pomodoro_phase[i] = int(ts.pomodoro_phase);
        cfg.timer_pomodoro_work_secs[i] = ts.pomodoro_work_elapsed.count();
    }
    if (hwnd) {
        RECT wr;
        if (IsIconic(hwnd)) {
            WINDOWPLACEMENT wp{};
            wp.length = sizeof(wp);
            GetWindowPlacement(hwnd, &wp);
            wr = wp.rcNormalPosition;
        } else {
            GetWindowRect(hwnd, &wr);
        }
        cfg.pos_valid = true;
        cfg.win_x = wr.left;
        cfg.win_y = wr.top;
        cfg.win_w = wr.right - wr.left;
    }
    config_write(cfg, f);
    f.close();
    if (f.good()) {
        std::error_code ec;
        std::filesystem::rename(tmp, path, ec);
        if (ec) dbg(L"[chrono] save_config: rename failed");
    } else {
        dbg(L"[chrono] save_config: write failed, keeping old config");
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
    }
}

void load_config(HWND hwnd, WndState& s) {
    using namespace std::chrono;
    const auto& path = s.cfg_path;
    dbg(std::format(L"[chrono] load_config: {}", path.wstring()));
    std::ifstream f(path);
    if (!f) {
        dbg(L"[chrono] no config, using defaults");
        return;
    }
    Config cfg;
    config_read(cfg, f);
    dbg(std::format(L"[chrono] loaded: clk={} sw={} tmr={} top={} pos={} ({},{} w={})", cfg.show_clk, cfg.show_sw,
                    cfg.show_tmr, cfg.topmost, cfg.pos_valid, cfg.win_x, cfg.win_y, cfg.win_w));
    s.app.show_clk = cfg.show_clk;
    s.app.clock_view = cfg.clock_view;
    s.app.show_sw = cfg.show_sw;
    s.app.show_tmr = cfg.show_tmr;
    s.app.show_alarms = cfg.show_alarms;
    s.app.alarms = cfg.alarms;
    s.app.alarm_notified_minute = -1;
    s.app.topmost = cfg.topmost;
    s.app.sound_on_expiry = cfg.sound_on_expiry;
    s.app.theme_mode = cfg.theme_mode;
    s.app.analog_style = cfg.analog_style;
    s.app.pomodoro_work_secs = cfg.pomodoro_work_secs;
    s.app.pomodoro_short_secs = cfg.pomodoro_short_secs;
    s.app.pomodoro_long_secs = cfg.pomodoro_long_secs;
    s.app.pomodoro_cadence = cfg.pomodoro_cadence;
    s.app.pomodoro_auto_start = cfg.pomodoro_auto_start;
    s.app.custom_preset_secs.clear();
    for (int i = 0; i < cfg.num_custom_presets; ++i)
        s.app.custom_preset_secs.push_back(cfg.custom_preset_secs[i]);
    int n = std::min(cfg.num_timers, Config::MAX_TIMERS);
    s.app.timers.resize(n);
    for (int i = 0; i < n; ++i) {
        s.app.timers[i].dur = seconds{cfg.timer_secs[i]};
        s.app.timers[i].t.reset();
        s.app.timers[i].t.set(s.app.timers[i].dur);
        s.app.timers[i].label = utf8_to_wide(cfg.timer_labels[i]);
    }
    auto now_steady = steady_clock::now();
    auto now_wall_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (cfg.sw_elapsed_ms > 0 || cfg.sw_running) {
        long long actual_ms = cfg.sw_elapsed_ms;
        if (cfg.sw_running && cfg.sw_start_epoch_ms > 0) {
            long long delta = now_wall_ms - cfg.sw_start_epoch_ms;
            if (delta > 0) actual_ms += delta;
        }
        actual_ms = std::max(actual_ms, 0LL);
        s.app.sw.restore(milliseconds{actual_ms}, cfg.sw_running, now_steady);
        if (!cfg.sw_lap_file.empty()) {
            auto p = std::filesystem::path{utf8_to_wide(cfg.sw_lap_file)};
            std::error_code ec;
            if (std::filesystem::exists(p, ec))
                s.app.sw_lap_file = std::move(p);
            else
                dbg(L"[chrono] load_config: sw_lap_file not found, clearing");
        }
    }
    for (int i = 0; i < n; ++i) {
        auto& ts = s.app.timers[i];
        ts.pomodoro = cfg.timer_pomodoro[i];
        ts.pomodoro_phase = cfg.timer_pomodoro_phase[i];
        ts.pomodoro_work_elapsed = std::chrono::seconds{cfg.timer_pomodoro_work_secs[i]};
        if (cfg.timer_elapsed_ms[i] <= 0 && !cfg.timer_running[i] && !cfg.timer_notified[i]) continue;
        long long elapsed_ms = cfg.timer_elapsed_ms[i];
        bool tmr_running = cfg.timer_running[i];
        if (tmr_running && cfg.timer_start_epoch_ms[i] > 0) {
            long long delta = now_wall_ms - cfg.timer_start_epoch_ms[i];
            if (delta > 0) elapsed_ms += delta;
        }
        elapsed_ms = std::max(elapsed_ms, 0LL);
        long long target_ms = duration_cast<milliseconds>(ts.dur).count();
        if (elapsed_ms >= target_ms) {
            elapsed_ms = target_ms;
            tmr_running = false;
        }
        ts.t.restore(ts.dur, milliseconds{elapsed_ms}, tmr_running, now_steady);
        ts.notified = cfg.timer_notified[i];
    }
    if (s.app.topmost) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if (cfg.pos_valid) {
        RECT cur;
        GetWindowRect(hwnd, &cur);
        DWORD ws = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
        RECT adj{0, 0, s.layout.bar_min_client_w(), 0};
        AdjustWindowRectEx(&adj, ws, FALSE, 0);
        int min_w = adj.right - adj.left;
        int w = cfg.win_w < min_w ? min_w : cfg.win_w;
        RECT test{cfg.win_x, cfg.win_y, cfg.win_x + w, cfg.win_y + 1};
        HMONITOR hmon = MonitorFromRect(&test, MONITOR_DEFAULTTONULL);
        if (hmon) {
            SetWindowPos(hwnd, nullptr, cfg.win_x, cfg.win_y, w, cur.bottom - cur.top, SWP_NOZORDER);
        } else {
            SetWindowPos(hwnd, nullptr, 0, 0, w, cur.bottom - cur.top, SWP_NOZORDER | SWP_NOMOVE);
        }
    }
}
