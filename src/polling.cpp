#include "polling.hpp"
#include <windows.h>
#include <chrono>
#include <format>
#include <string>
#include "app.hpp"
#include "encoding.hpp"
#include "config_io.hpp"
#include "formatting.hpp"
#include "pomodoro.hpp"
#include "tray.hpp"
#include "wndstate.hpp"

void sync_timer(HWND hwnd, WndState& s) {
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

void update_title(HWND hwnd, WndState& s) {
    using namespace std::chrono;
    auto now = steady_clock::now();
    std::wstring title;
    int n = (int)s.app.timers.size();
    for (int i = 0; i < n; ++i) {
        auto& ts = s.app.timers[i];
        if (ts.t.is_running()) {
            title = format_timer_title(ts.label, i, n,
                format_timer_display(ts.t.remaining(now)), ts.t.expired(now));
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
    title += L" — Chronos";
    if (now < s.clipboard_copied_until) title = L"Copied — Chronos";
    SetWindowTextW(hwnd, title.c_str());
    if (s.tray_active) tray_update_tip(hwnd, title.c_str());
}

void check_alarms(HWND hwnd, WndState& s) {
    auto& app = s.app;
    if (app.alarms.empty()) return;

    SYSTEMTIME st; GetLocalTime(&st);
    int current_minute = st.wHour * 60 + st.wMinute;

    // Reset notified flags at the start of each new minute
    if (current_minute != app.alarm_notified_minute) {
        app.alarm_notified_minute = current_minute;
        for (auto& a : app.alarms) a.notified = false;
    }

    // SYSTEMTIME wDayOfWeek: 0=Sunday, 1=Monday, ..., 6=Saturday
    // Our bitmask: bit 0=Monday ... bit 6=Sunday
    int dow_sys = st.wDayOfWeek; // 0=Sun
    int dow_bit = (dow_sys == 0) ? 6 : (dow_sys - 1); // Mon=0 ... Sun=6

    for (auto& a : app.alarms) {
        if (!a.enabled) continue;
        if (a.notified) continue;
        if (a.hour != st.wHour || a.minute != st.wMinute) continue;

        bool matches = false;
        if (a.schedule == AlarmSchedule::Days) {
            matches = (a.days_mask & (1 << dow_bit)) != 0;
        } else {
            matches = (a.date_year  == st.wYear  &&
                       a.date_month == st.wMonth &&
                       a.date_day   == st.wDay);
        }
        if (!matches) continue;

        a.notified = true;
        MessageBeep(MB_ICONASTERISK);
        FLASHWINFO fi{sizeof(fi), hwnd, FLASHW_ALL | FLASHW_TIMERNOFG, 3, 0};
        FlashWindowEx(&fi);

        if (s.tray_active) {
            std::wstring name_w = utf8_to_wide(a.name);
            const wchar_t* title = L"Alarm";
            tray_notify(hwnd, title, name_w.empty() ? title : name_w.c_str());
        }
    }
}

void handle_wm_timer(HWND hwnd, WndState& s) {
    using namespace std::chrono;
    auto now = steady_clock::now();
    int n = (int)s.app.timers.size();
    for (int i = 0; i < n; ++i) {
        auto& ts = s.app.timers[i];
        if (ts.t.touched() && ts.t.expired(now) && !ts.notified) {
            ts.notified = true;
            if (s.app.sound_on_expiry) MessageBeep(MB_ICONASTERISK);
            FLASHWINFO fi{};
            fi.cbSize = sizeof(fi);
            fi.hwnd = hwnd;
            fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            fi.uCount = 3;
            fi.dwTimeout = 0;
            FlashWindowEx(&fi);
            if (s.tray_active) {
                auto tray_t = format_tray_title(i, n);
                auto lbl = ts.label.empty() ? L"Timer" : ts.label.c_str();
                tray_notify(hwnd, tray_t.c_str(), lbl);
            }
            if (ts.pomodoro) {
                advance_pomodoro_phase(ts, s.app.pomodoro_work_secs,
                    s.app.pomodoro_short_secs, s.app.pomodoro_long_secs,
                    s.app.pomodoro_cadence, s.app.pomodoro_auto_start, now);
                save_config(hwnd, s);
            }
        }
    }
    check_alarms(hwnd, s);
    update_title(hwnd, s);
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd, s);
}
