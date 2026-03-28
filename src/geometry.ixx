module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
export module geometry;
import layout;
import wndstate;

static LayoutState layout_state(const WndState& s) {
    return {
        .show_clk = s.app.show_clk,
        .show_sw = s.app.show_sw,
        .show_tmr = s.app.show_tmr,
        .timer_count = (int)s.app.timers.size(),
    };
}

export int client_height(const WndState& s) { return client_height_for(s.layout, layout_state(s)); }

export int nonclient_height(HWND hwnd) {
    RECT wr, cr;
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);
    return (wr.bottom - wr.top) - cr.bottom;
}

export void resize_window(HWND hwnd, const WndState& s) {
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int cur_w = wr.right - wr.left;
    SetWindowPos(hwnd, nullptr, 0, 0, cur_w, client_height(s) + nonclient_height(hwnd), SWP_NOMOVE | SWP_NOZORDER);
}

export int timer_index_at_y(const WndState& s, int y) {
    if (!s.app.show_tmr) return -1;
    int top = s.layout.bar_h;
    if (s.app.show_clk) top += s.layout.clk_h;
    if (s.app.show_sw) top += s.layout.sw_h;
    if (y < top) return -1;
    int idx = (y - top) / s.layout.tmr_h;
    return idx < (int)s.app.timers.size() ? idx : -1;
}
