#pragma once
#include <windows.h>
#include <chrono>
#include <format>
#include <string>
#include "app.hpp"
#include "config_io.hpp"
#include "formatting.hpp"
#include "pomodoro.hpp"
#include "tray.hpp"
#include "wndstate.hpp"

constexpr int POLL_STOPWATCH_MS = 20;
constexpr int POLL_TIMER_MS = 100;
constexpr int POLL_IDLE_MS = 1000;

inline void sync_timer(HWND hwnd, WndState& s) {
    bool any_timer_running = false;
    for (auto& ts : s.app.timers)
        if (ts.t.is_running()) {
            any_timer_running = true;
            break;
        }
    int want = (s.app.show_sw && s.app.sw.is_running()) ? POLL_STOPWATCH_MS
               : any_timer_running                      ? POLL_TIMER_MS
                                                        : POLL_IDLE_MS;
    if (want != s.timer_ms) {
        s.timer_ms = want;
        SetTimer(hwnd, 1, want, nullptr);
    }
}

inline void handle_wm_timer(HWND hwnd, WndState& s) {
    using namespace std::chrono;
    auto now = steady_clock::now();
    for (auto& ts : s.app.timers) {
        if (ts.t.touched() && ts.t.expired(now) && !ts.notified) {
            ts.notified = true;
            MessageBeep(MB_ICONASTERISK);
            FLASHWINFO fi{};
            fi.cbSize = sizeof(fi);
            fi.hwnd = hwnd;
            fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            fi.uCount = 3;
            fi.dwTimeout = 0;
            FlashWindowEx(&fi);
            if (s.tray_active) {
                auto lbl = ts.label.empty() ? L"Timer" : ts.label.c_str();
                tray_notify(hwnd, L"Timer expired", lbl);
            }
            if (ts.pomodoro) {
                ts.pomodoro_phase = (ts.pomodoro_phase + 1) % 8;
                auto secs = std::chrono::seconds{pomodoro_phase_secs(ts.pomodoro_phase,
                    s.app.pomodoro_work_secs, s.app.pomodoro_short_secs, s.app.pomodoro_long_secs)};
                ts.dur = secs;
                ts.t.reset();
                ts.t.set(secs);
                ts.t.start(now);
                ts.notified = false;
                ts.label = pomodoro_phase_label(ts.pomodoro_phase);
                save_config(hwnd, s);
            }
        }
    }
    {
        std::wstring title;
        for (auto& ts : s.app.timers) {
            if (ts.t.is_running()) {
                title = format_timer_display(ts.t.remaining(now));
                if (ts.t.expired(now)) title = L"EXPIRED " + title;
                break;
            }
        }
        if (title.empty() && s.app.sw.is_running()) {
            title = format_stopwatch_display(s.app.sw.elapsed(now));
        }
        if (title.empty()) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            title = std::format(L"{:02}:{:02}:{:02}", st.wHour, st.wMinute, st.wSecond);
        }
        title += L" \u2014 Chronos";
        SetWindowTextW(hwnd, title.c_str());
        if (s.tray_active) tray_update_tip(hwnd, title.c_str());
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd, s);
}
