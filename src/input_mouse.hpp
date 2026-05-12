#pragma once
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <chrono>
#include <format>
#include <optional>
#include "actions.hpp"
#include "config.hpp"
#include "config_io.hpp"
#include "geometry.hpp"
#include "gdi.hpp"
#include "input_core.hpp"
#include "input_label_edit.hpp"
#include "layout.hpp"
#include "pomodoro.hpp"
#include "pomodoro_dialog.hpp"
#include "timer_presets.hpp"
#include "wndstate.hpp"

inline std::optional<LRESULT> dispatch_mouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, WndState& s) {
    switch (msg) {
    case WM_LBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        for (auto& [r, id] : s.btns)
            if (PtInRect(&r, pt)) {
                handle(hwnd, id, s);
                break;
            }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        for (auto& [r, id] : s.btns)
            if (PtInRect(&r, pt)) {
                handle(hwnd, id, s);
                return 0;
            }
        try_start_label_edit(hwnd, pt, s);
        return 0;
    }
    case WM_COMMAND:
        handle_label_edit_command(hwnd, wp, lp, s);
        return 0;
    case WM_RBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        int idx = timer_index_at_y(s.layout, layout_state(s), pt.y);
        if (idx >= 0 && !s.app.timers[idx].t.touched()) {
            constexpr int CMD_POMODORO = 100;
            constexpr int CMD_POMO_CFG = 101;
            HMENU menu = CreatePopupMenu();
            for (int i = 0; i < (int)std::size(TIMER_PRESETS); ++i) AppendMenuW(menu, MF_STRING, 1 + i, TIMER_PRESETS[i].label);
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            int pom_min = s.app.pomodoro_work_secs / 60;
            int pom_sec = s.app.pomodoro_work_secs % 60;
            auto pom_label = pom_sec > 0
                ? std::format(L"Pomodoro ({:02}:{:02})", pom_min, pom_sec)
                : std::format(L"Pomodoro ({}:00)", pom_min);
            AppendMenuW(menu, MF_STRING, CMD_POMODORO, pom_label.c_str());
            AppendMenuW(menu, MF_STRING, CMD_POMO_CFG, L"Edit Pomodoro intervals…");
            POINT scr = pt;
            ClientToScreen(hwnd, &scr);
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, scr.x, scr.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            if (cmd > 0) {
                auto& ts = s.app.timers[idx];
                if (cmd == CMD_POMODORO) {
                    ts.pomodoro = true;
                    ts.pomodoro_phase = 0;
                    ts.dur = std::chrono::seconds{s.app.pomodoro_work_secs};
                    ts.label = pomodoro_phase_label(0, s.app.pomodoro_cadence);
                    ts.notified = false;
                    ts.t.reset();
                    ts.t.set(ts.dur);
                    save_config(hwnd, s);
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (cmd == CMD_POMO_CFG) {
                    if (show_pomodoro_interval_dialog(hwnd, s.app, (HFONT)s.fontSm.h,
                                                       s.active_theme, s.layout.dpi)) {
                        save_config(hwnd, s);
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                } else {
                    ts.pomodoro = false;
                    ts.label.clear();
                    ts.dur = std::chrono::seconds{TIMER_PRESETS[cmd - 1].secs};
                    ts.notified = false;
                    ts.t.reset();
                    ts.t.set(ts.dur);
                    save_config(hwnd, s);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &pt);
        int idx = timer_index_at_y(s.layout, layout_state(s), pt.y);
        if (idx >= 0 && !s.app.timers[idx].t.touched() && !s.app.timers[idx].pomodoro) {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            bool up = delta > 0;
            RECT cr;
            GetClientRect(hwnd, &cr);
            int cw = cr.right;
            int col_gap = TimerMetrics::from(s.layout).col_gap;
            int sep1 = cw / 2 - col_gap / 2;
            int sep2 = cw / 2 + col_gap / 2;
            int off;
            if (pt.x < sep1)
                off = up ? A_TMR_HUP : A_TMR_HDN;
            else if (pt.x < sep2)
                off = up ? A_TMR_MUP : A_TMR_MDN;
            else
                off = up ? A_TMR_SUP : A_TMR_SDN;
            handle(hwnd, tmr_act(idx, off), s);
        }
        return 0;
    }
    default:
        return std::nullopt;
    }
}
