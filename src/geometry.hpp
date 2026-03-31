#pragma once
#include <windows.h>
#include "layout.hpp"
#include "wndstate.hpp"

inline LayoutState layout_state(const WndState& s) {
    return {
        .show_clk = s.app.show_clk,
        .show_sw = s.app.show_sw,
        .show_tmr = s.app.show_tmr,
        .timer_count = (int)s.app.timers.size(),
    };
}

inline int client_height(const WndState& s) { return client_height_for(s.layout, layout_state(s)); }

inline int nonclient_height(HWND hwnd) {
    RECT wr, cr;
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);
    return (wr.bottom - wr.top) - cr.bottom;
}

inline void resize_window(HWND hwnd, const WndState& s) {
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int cur_w = wr.right - wr.left;
    SetWindowPos(hwnd, nullptr, 0, 0, cur_w, client_height(s) + nonclient_height(hwnd), SWP_NOMOVE | SWP_NOZORDER);
}
