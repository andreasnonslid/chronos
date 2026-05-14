#pragma once
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include "assert.hpp"
#include "app.hpp"
#include "config.hpp"
#include "formatting.hpp"
#include "pomodoro.hpp"
#include "timer.hpp"

enum Act {
    A_TOPMOST = 1,
    A_SHOW_CLK,
    A_SHOW_SW,
    A_SHOW_TMR,
    A_SW_START,
    A_SW_LAP,
    A_SW_RESET,
    A_SW_GET,
    A_SW_COPY,
    A_THEME,
    A_CLK_CYCLE,
    A_TMR_RST_ALL,
    A_SETTINGS,
    A_SHOW_ALARMS,
    A_ALARM_ADD,
    A_TMR_BASE    = 100,
    A_ALARM_DEL   = 500,  // A_ALARM_DEL + i to delete alarm i
    A_ALARM_TOGGLE = 520, // A_ALARM_TOGGLE + i to enable/disable alarm i
};

constexpr int TMR_STRIDE = 12;
constexpr int A_TMR_HUP = 0;
constexpr int A_TMR_HDN = 1;
constexpr int A_TMR_MUP = 2;
constexpr int A_TMR_MDN = 3;
constexpr int A_TMR_SUP = 4;
constexpr int A_TMR_SDN = 5;
constexpr int A_TMR_START = 6;
constexpr int A_TMR_RST = 7;
constexpr int A_TMR_ADD = 8;
constexpr int A_TMR_DEL = 9;
constexpr int A_TMR_POMO = 10;
constexpr int A_TMR_SKIP = 11;

inline void reset_timer_slot(TimerSlot& ts, const App& app) {
    ts.notified = false;
    if (ts.pomodoro) {
        ts.pomodoro_phase = 0;
        ts.dur = std::chrono::seconds{app.pomodoro_work_secs};
        ts.label = pomodoro_phase_label(0, app.pomodoro_cadence);
        ts.pomodoro_work_elapsed = {};
    }
    ts.t.reset();
    ts.t.set(ts.dur);
}

inline bool apply_timer_hms_adjust(TimerSlot& ts, int off) {
    auto total = ts.dur.count();
    int h = (int)(total / 3600);
    int m = (int)((total / 60) % 60);
    int sec = (int)(total % 60);
    struct Adj { int off; int* field; int max_val; bool up; };
    Adj adjustments[] = {
        {A_TMR_HUP, &h, 24, true},  {A_TMR_HDN, &h, 24, false},
        {A_TMR_MUP, &m, 59, true},  {A_TMR_MDN, &m, 59, false},
        {A_TMR_SUP, &sec, 59, true}, {A_TMR_SDN, &sec, 59, false},
    };
    bool changed = false;
    for (auto& [a_off, field, max_val, up] : adjustments) {
        if (off == a_off) {
            *field = up ? (*field >= max_val ? 0 : *field + 1)
                        : (*field <= 0 ? max_val : *field - 1);
            changed = true;
            break;
        }
    }
    if (changed) {
        if (h == 24) { m = 0; sec = 0; }
        auto clamped = std::clamp(h * 3600 + m * 60 + sec, 0, Config::TIMER_MAX_SECS);
        ts.dur = std::chrono::seconds{clamped};
        ts.t.reset();
        ts.t.set(ts.dur);
    }
    return changed;
}

inline int tmr_act(int i, int off) {
    CHRONOS_ASSERT(i >= 0 && off >= 0 && off < TMR_STRIDE);
    return A_TMR_BASE + i * TMR_STRIDE + off;
}

inline bool wants_blink(int act) {
    switch (act) {
    case A_TOPMOST:
    case A_SHOW_CLK:
    case A_SHOW_SW:
    case A_SHOW_TMR:
    case A_SHOW_ALARMS:
    case A_ALARM_ADD:
    case A_SW_START:
    case A_SW_COPY:
    case A_THEME:
    case A_CLK_CYCLE:
    case A_SETTINGS:
        return false;
    default:
        if (act >= A_ALARM_DEL && act < A_ALARM_DEL + ALARM_MAX_COUNT) return false;
        if (act >= A_ALARM_TOGGLE && act < A_ALARM_TOGGLE + ALARM_MAX_COUNT) return false;
        if (act >= A_TMR_BASE && act < A_ALARM_DEL) {
            int off = (act - A_TMR_BASE) % TMR_STRIDE;
            if (off == A_TMR_START || off == A_TMR_ADD || off == A_TMR_DEL) return false;
        }
        return true;
    }
}

struct HandleResult {
    bool save_config = false;
    bool resize = false;
    bool set_topmost = false;
    bool open_file = false;
    bool copy_laps = false;
    bool apply_theme = false;
    bool open_settings = false;
    bool open_alarm_dialog = false;
};

inline HandleResult dispatch_action(App& app, int act, std::chrono::steady_clock::time_point now,
                                     const std::filesystem::path& config_dir) {
    using namespace std::chrono;
    HandleResult r;

    switch (act) {
    case A_TOPMOST:
        app.topmost = !app.topmost;
        r.set_topmost = true;
        r.save_config = true;
        break;
    case A_SHOW_CLK:
        app.show_clk = !app.show_clk;
        r.resize = true;
        r.save_config = true;
        break;
    case A_SHOW_SW:
        app.show_sw = !app.show_sw;
        r.resize = true;
        r.save_config = true;
        break;
    case A_SHOW_TMR:
        app.show_tmr = !app.show_tmr;
        r.resize = true;
        r.save_config = true;
        break;
    case A_SW_START:
        if (!app.sw.is_running()) {
            if (app.sw_lap_file.empty()) {
                auto tp = std::chrono::system_clock::now();
                auto days = std::chrono::floor<std::chrono::days>(tp);
                std::chrono::year_month_day ymd{days};
                std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::milliseconds>(tp - days)};
                auto lap_name = std::format(L"stopwatch-{:04}{:02}{:02}-{:02}{:02}{:02}-{:03}.txt", (int)ymd.year(),
                                            (unsigned)ymd.month(), (unsigned)ymd.day(), hms.hours().count(),
                                            hms.minutes().count(), hms.seconds().count(), hms.subseconds().count());
                app.sw_lap_file = config_dir / lap_name;
            }
            app.sw.start(now);
        } else {
            app.sw.stop(now);
        }
        break;
    case A_SW_LAP:
        if (app.sw.is_running()) {
            app.sw.lap(now);
            if (!app.sw_lap_file.empty()) {
                std::wofstream f(app.sw_lap_file, std::ios::app);
                if (f) {
                    const auto& laps = app.sw.laps();
                    auto n = laps.size();
                    f << format_lap_row(n, laps.back(), app.sw.cumulative()) << L'\n';
                    app.lap_write_failed = !f.good();
                } else {
                    app.lap_write_failed = true;
                }
            }
        }
        break;
    case A_SW_RESET:
        app.sw.reset();
        app.sw_lap_file.clear();
        break;
    case A_SW_GET:
        if (!app.sw_lap_file.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(app.sw_lap_file, ec))
                r.open_file = true;
            else
                app.sw_lap_file.clear();
        }
        break;
    case A_SW_COPY:
        if (!app.sw.laps().empty()) r.copy_laps = true;
        break;
    case A_THEME:
        app.theme_mode = (ThemeMode)(((int)app.theme_mode + 1) % THEME_MODE_COUNT);
        r.apply_theme = true;
        r.save_config = true;
        break;
    case A_CLK_CYCLE: {
        auto old_view = app.clock_view;
        app.clock_view = (ClockView)(((int)app.clock_view + 1) % CLOCK_VIEW_COUNT);
        if (old_view == ClockView::Analog || app.clock_view == ClockView::Analog)
            r.resize = true;
        r.save_config = true;
        break;
    }
    case A_SETTINGS:
        r.open_settings = true;
        break;
    case A_SHOW_ALARMS:
        app.show_alarms = !app.show_alarms;
        r.resize = true;
        r.save_config = true;
        break;
    case A_ALARM_ADD:
        r.open_alarm_dialog = true;
        break;
    case A_TMR_RST_ALL:
        for (auto& ts : app.timers)
            reset_timer_slot(ts, app);
        r.save_config = true;
        break;
    default:
        if (act >= A_ALARM_DEL && act < A_ALARM_DEL + ALARM_MAX_COUNT) {
            int i = act - A_ALARM_DEL;
            if (i >= 0 && i < (int)app.alarms.size()) {
                app.alarms.erase(app.alarms.begin() + i);
                r.resize = true;
                r.save_config = true;
            }
        } else if (act >= A_ALARM_TOGGLE && act < A_ALARM_TOGGLE + ALARM_MAX_COUNT) {
            int i = act - A_ALARM_TOGGLE;
            if (i >= 0 && i < (int)app.alarms.size()) {
                app.alarms[i].enabled = !app.alarms[i].enabled;
                r.save_config = true;
            }
        } else if (act >= A_TMR_BASE) {
            int idx = (act - A_TMR_BASE) / TMR_STRIDE;
            int off = (act - A_TMR_BASE) % TMR_STRIDE;
            if (idx < 0 || idx >= (int)app.timers.size()) break;
            auto& ts = app.timers[idx];
            if (off == A_TMR_START) {
                if (!ts.t.touched()) {
                    if (ts.dur.count() > 0) ts.t.start(now);
                } else if (ts.t.is_running())
                    ts.t.pause(now);
                else
                    ts.t.start(now);
            } else if (off == A_TMR_RST) {
                reset_timer_slot(ts, app);
            } else if (off == A_TMR_ADD) {
                if ((int)app.timers.size() < Config::MAX_TIMERS) {
                    TimerSlot ns;
                    ns.t.set(ns.dur);
                    app.timers.insert(app.timers.begin() + idx + 1, ns);
                    r.resize = true;
                    r.save_config = true;
                }
            } else if (off == A_TMR_DEL) {
                if ((int)app.timers.size() > 1) {
                    app.timers.erase(app.timers.begin() + idx);
                    r.resize = true;
                    r.save_config = true;
                }
            } else if (off == A_TMR_POMO) {
                if (!ts.t.touched()) {
                    if (ts.pomodoro) {
                        ts.pomodoro = false;
                        ts.label.clear();
                    } else {
                        ts.pomodoro = true;
                        ts.pomodoro_phase = 0;
                        ts.pomodoro_work_elapsed = {};
                        ts.dur = seconds{app.pomodoro_work_secs};
                        ts.label = pomodoro_phase_label(0, app.pomodoro_cadence);
                        ts.notified = false;
                        ts.t.reset();
                        ts.t.set(ts.dur);
                    }
                    r.save_config = true;
                }
            } else if (off == A_TMR_SKIP) {
                if (ts.pomodoro && ts.t.touched()) {
                    if (pomodoro_is_work(ts.pomodoro_phase)) {
                        auto elapsed = duration_cast<seconds>(ts.t.total_elapsed(now));
                        ts.pomodoro_work_elapsed += elapsed;
                    }
                    ts.pomodoro_phase = pomodoro_next_phase(ts.pomodoro_phase, app.pomodoro_cadence);
                    auto secs = seconds{pomodoro_phase_secs(ts.pomodoro_phase,
                        app.pomodoro_work_secs, app.pomodoro_short_secs, app.pomodoro_long_secs,
                        app.pomodoro_cadence)};
                    ts.dur = secs;
                    ts.notified = false;
                    ts.t.reset();
                    ts.t.set(secs);
                    ts.t.start(now);
                    ts.label = pomodoro_phase_label(ts.pomodoro_phase, app.pomodoro_cadence);
                    r.save_config = true;
                }
            } else if (!ts.t.touched() && apply_timer_hms_adjust(ts, off)) {
                r.save_config = true;
            }
        }
    }
    return r;
}
