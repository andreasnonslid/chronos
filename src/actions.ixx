module;
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
export module actions;
import app;
import config;
import formatting;
import timer;

using namespace std::chrono;
using sc = steady_clock;

export enum Act {
    A_TOPMOST = 1,
    A_SHOW_CLK,
    A_SHOW_SW,
    A_SHOW_TMR,
    A_SW_START,
    A_SW_LAP,
    A_SW_RESET,
    A_SW_GET,
    A_TMR_BASE = 100,
};

export constexpr int TMR_STRIDE = 10;
export constexpr int A_TMR_HUP = 0;
export constexpr int A_TMR_HDN = 1;
export constexpr int A_TMR_MUP = 2;
export constexpr int A_TMR_MDN = 3;
export constexpr int A_TMR_SUP = 4;
export constexpr int A_TMR_SDN = 5;
export constexpr int A_TMR_START = 6;
export constexpr int A_TMR_RST = 7;
export constexpr int A_TMR_ADD = 8;
export constexpr int A_TMR_DEL = 9;

export int tmr_act(int i, int off) { return A_TMR_BASE + i * TMR_STRIDE + off; }

export bool wants_blink(int act) {
    switch (act) {
    case A_TOPMOST:
    case A_SHOW_CLK:
    case A_SHOW_SW:
    case A_SHOW_TMR:
    case A_SW_START:
        return false;
    default:
        if (act >= A_TMR_BASE) {
            int off = (act - A_TMR_BASE) % TMR_STRIDE;
            if (off == A_TMR_START || off == A_TMR_ADD || off == A_TMR_DEL) return false;
        }
        return true;
    }
}

export struct HandleResult {
    bool save_config = false;
    bool resize = false;
    bool set_topmost = false;
    bool open_file = false;
};

export HandleResult dispatch_action(App& app, int act, sc::time_point now, const std::filesystem::path& config_dir) {
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
                SYSTEMTIME st;
                GetLocalTime(&st);
                auto lap_name = std::format(L"stopwatch-{:04}{:02}{:02}-{:02}{:02}{:02}-{:03}.txt", st.wYear, st.wMonth,
                                            st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                app.sw_lap_file = (config_dir / lap_name).wstring();
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
                std::wofstream f(app.sw_lap_file.c_str(), std::ios::app);
                if (f) {
                    const auto& laps = app.sw.laps();
                    auto n = laps.size();
                    f << format_lap_row(n, laps.back(), app.sw.cumulative()) << L'\n';
                    app.lap_write_failed = false;
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
        if (!app.sw_lap_file.empty()) r.open_file = true;
        break;
    default:
        if (act >= A_TMR_BASE) {
            int idx = (act - A_TMR_BASE) / TMR_STRIDE;
            int off = (act - A_TMR_BASE) % TMR_STRIDE;
            if (idx < 0 || idx >= (int)app.timers.size()) break;
            auto& ts = app.timers[idx];
            if (off == A_TMR_START) {
                if (!ts.t.touched())
                    ts.t.start(now);
                else if (ts.t.is_running())
                    ts.t.pause(now);
                else
                    ts.t.start(now);
            } else if (off == A_TMR_RST) {
                ts.t.reset();
                ts.t.set(ts.dur);
                ts.notified = false;
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
            } else if (!ts.t.touched()) {
                auto total = ts.dur.count();
                int h = (int)(total / 3600);
                int m = (int)((total / 60) % 60);
                int sec = (int)(total % 60);
                bool changed = true;
                if (off == A_TMR_HUP)
                    h = (h >= 24) ? 0 : h + 1;
                else if (off == A_TMR_HDN)
                    h = (h <= 0) ? 24 : h - 1;
                else if (off == A_TMR_MUP)
                    m = (m >= 59) ? 0 : m + 1;
                else if (off == A_TMR_MDN)
                    m = (m <= 0) ? 59 : m - 1;
                else if (off == A_TMR_SUP)
                    sec = (sec >= 59) ? 0 : sec + 1;
                else if (off == A_TMR_SDN)
                    sec = (sec <= 0) ? 59 : sec - 1;
                else
                    changed = false;
                if (changed) {
                    ts.dur = seconds{h * 3600 + m * 60 + sec};
                    ts.t.reset();
                    ts.t.set(ts.dur);
                    r.save_config = true;
                }
            }
        }
    }
    return r;
}
