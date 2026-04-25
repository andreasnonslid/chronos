#pragma once
#include <windows.h>
#include "config_io.hpp"
#include "geometry.hpp"
#include "wndstate.hpp"

constexpr int EDIT_ID_BASE = 9000;

inline LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* s = (WndState*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
    if (msg == WM_KEYDOWN) {
        if (wp == VK_RETURN) {
            SetFocus(GetParent(hwnd));
            return 0;
        }
        if (wp == VK_ESCAPE) {
            s->edit_cancelled = true;
            SetFocus(GetParent(hwnd));
            return 0;
        }
    }
    return CallWindowProcW(s->orig_edit_proc, hwnd, msg, wp, lp);
}

inline void try_start_label_edit(HWND hwnd, POINT pt, WndState& s) {
    int idx = timer_index_at_y(s.layout, layout_state(s), pt.y);
    if (idx < 0) return;
    auto& ts = s.app.timers[idx];
    wchar_t buf[Config::MAX_LABEL_LEN + 1] = {};
    if (!ts.label.empty()) wcsncpy(buf, ts.label.c_str(), Config::MAX_LABEL_LEN);
    RECT dlg_r;
    GetClientRect(hwnd, &dlg_r);
    int top = s.layout.bar_h;
    if (s.app.show_clk) top += s.layout.clk_h;
    if (s.app.show_sw) top += s.layout.sw_h;
    int ey = top + idx * s.layout.tmr_h + s.layout.dpi_scale(2);
    int eh = s.layout.dpi_scale(18);
    int ew = s.layout.dpi_scale(120);
    int ex = (dlg_r.right - ew) / 2;
    HWND edit = CreateWindowExW(0, L"EDIT", buf,
                                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
                                ex, ey, ew, eh, hwnd, (HMENU)(INT_PTR)(EDIT_ID_BASE + idx), nullptr, nullptr);
    if (!edit) return;
    SendMessageW(edit, EM_SETLIMITTEXT, Config::MAX_LABEL_LEN, 0);
    SendMessageW(edit, WM_SETFONT, (WPARAM)(HFONT)s.fontSm.h, TRUE);
    s.edit_cancelled = false;
    s.orig_edit_proc = (WNDPROC)SetWindowLongPtrW(edit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
    SetFocus(edit);
    SendMessageW(edit, EM_SETSEL, 0, -1);
}

inline bool handle_label_edit_command(HWND hwnd, WPARAM wp, LPARAM lp, WndState& s) {
    int id = LOWORD(wp);
    int code = HIWORD(wp);
    if (id >= EDIT_ID_BASE && id < EDIT_ID_BASE + Config::MAX_TIMERS && code == EN_KILLFOCUS) {
        int idx = id - EDIT_ID_BASE;
        HWND edit = (HWND)lp;
        if (!s.edit_cancelled) {
            wchar_t buf[Config::MAX_LABEL_LEN + 1] = {};
            GetWindowTextW(edit, buf, Config::MAX_LABEL_LEN + 1);
            if (idx < (int)s.app.timers.size()) {
                s.app.timers[idx].label = buf;
                save_config(hwnd, s);
            }
        }
        s.edit_cancelled = false;
        DestroyWindow(edit);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    return false;
}
