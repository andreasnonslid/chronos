#pragma once
#include <algorithm>
#include <windows.h>
#include "layout.hpp"
#include "wndstate.hpp"

inline LayoutState layout_state(const WndState& s) {
    return {
        .show_clk = s.app.show_clk,
        .show_sw = s.app.show_sw,
        .show_tmr = s.app.show_tmr,
        .show_alarms = s.app.show_alarms,
        .timer_count = (int)s.app.timers.size(),
        .alarm_count = (int)s.app.alarms.size(),
        .clock_view = s.app.clock_view,
        .analog_radius_pct = s.app.analog_style.radius_pct,
    };
}

inline int client_height(const WndState& s) { return client_height_for(s.layout, layout_state(s)); }

inline int nonclient_height(HWND hwnd) {
    RECT wr, cr;
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);
    return (wr.bottom - wr.top) - cr.bottom;
}

// For an oversized analog clock, the painted radius is bounded by min(w,h)/2,
// so leaving the window at its current width would cap visible growth. Pick a
// width that lets the clock actually reach the requested radius_pct.
inline int min_client_w_for(const WndState& s) {
    int w = s.layout.bar_min_client_w();
    if (s.app.show_clk && clock_view_has_analog(s.app.clock_view) &&
        s.app.analog_style.radius_pct > 100) {
        int analog_h = effective_clk_h(s.layout, s.app.clock_view, s.app.analog_style.radius_pct);
        // Analog occupies half the width in mixed-with-analog views.
        int needed = (s.app.clock_view == ClockView::Analog) ? analog_h : analog_h * 2;
        w = std::max(w, needed);
    }
    return w;
}

inline void resize_window(HWND hwnd, const WndState& s) {
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int cur_w = wr.right - wr.left;
    int cur_h = wr.bottom - wr.top;
    RECT cr;
    GetClientRect(hwnd, &cr);
    int nonclient_w = cur_w - cr.right;
    int min_w = min_client_w_for(s) + nonclient_w;
    int new_w = min_w;
    int min_h = client_height(s) + nonclient_height(hwnd);
    int new_h = std::max(cur_h, min_h);
    SetWindowPos(hwnd, nullptr, 0, 0, new_w, new_h, SWP_NOMOVE | SWP_NOZORDER);
}
