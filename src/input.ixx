module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <algorithm>
#include <chrono>
#include <optional>
extern "C" __declspec(dllimport) HRESULT __stdcall DwmSetWindowAttribute(HWND hwnd, DWORD attr, LPCVOID data, DWORD size);
export module input;
import actions;
import config;
import config_io;
import geometry;
import gdi;
import icon;
import layout;
import polling;
import theme;
import tray;
import wndstate;

using namespace std::chrono;
using sc = steady_clock;

// ─── Action wrapper ───────────────────────────────────────────────────────────
export void handle(HWND hwnd, int act, WndState& s) {
    auto now = sc::now();
    if (wants_blink(act)) {
        s.app.blink_act = act;
        s.app.blink_t = now;
    }

    auto r = dispatch_action(s.app, act, now, s.cfg_path.parent_path());

    if (r.set_topmost)
        SetWindowPos(hwnd, s.app.topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if (r.resize) resize_window(hwnd, s);
    if (r.save_config) save_config(hwnd, s);
    if (r.open_file) ShellExecuteW(nullptr, L"open", s.app.sw_lap_file.c_str(), nullptr, nullptr, SW_SHOW);
    if (r.apply_theme) apply_theme(hwnd, s);
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd, s);
}

static constexpr int EDIT_ID_BASE = 9000;

static WNDPROC g_orig_edit_proc = nullptr;
static bool g_edit_cancelled = false;

static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_KEYDOWN) {
        if (wp == VK_RETURN) {
            SetFocus(GetParent(hwnd));
            return 0;
        }
        if (wp == VK_ESCAPE) {
            g_edit_cancelled = true;
            SetFocus(GetParent(hwnd));
            return 0;
        }
    }
    return CallWindowProcW(g_orig_edit_proc, hwnd, msg, wp, lp);
}

// ─── Input message dispatcher ─────────────────────────────────────────────────
export std::optional<LRESULT> dispatch_input(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, WndState& s) {
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
        int idx = timer_index_at_y(s, pt.y);
        if (idx >= 0) {
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
            HWND edit = CreateWindowExW(0, L"EDIT", buf, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
                                        ex, ey, ew, eh, hwnd, (HMENU)(INT_PTR)(EDIT_ID_BASE + idx), nullptr, nullptr);
            SendMessageW(edit, EM_SETLIMITTEXT, Config::MAX_LABEL_LEN, 0);
            SendMessageW(edit, WM_SETFONT, (WPARAM)s.res.fontSm, TRUE);
            g_edit_cancelled = false;
            g_orig_edit_proc = (WNDPROC)SetWindowLongPtrW(edit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
            SetFocus(edit);
            SendMessageW(edit, EM_SETSEL, 0, -1);
        }
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp);
        int code = HIWORD(wp);
        if (id >= EDIT_ID_BASE && id < EDIT_ID_BASE + Config::MAX_TIMERS && code == EN_KILLFOCUS) {
            int idx = id - EDIT_ID_BASE;
            HWND edit = (HWND)lp;
            if (!g_edit_cancelled) {
                wchar_t buf[Config::MAX_LABEL_LEN + 1] = {};
                GetWindowTextW(edit, buf, Config::MAX_LABEL_LEN + 1);
                if (idx < (int)s.app.timers.size()) {
                    s.app.timers[idx].label = buf;
                    save_config(hwnd, s);
                }
            }
            g_edit_cancelled = false;
            DestroyWindow(edit);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_RBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        int idx = timer_index_at_y(s, pt.y);
        if (idx >= 0 && !s.app.timers[idx].t.touched()) {
            struct {
                int secs;
                const wchar_t* label;
            } presets[] = {
                {60, L"1:00"},    {120, L"2:00"},   {180, L"3:00"},     {300, L"5:00"},
                {600, L"10:00"},  {900, L"15:00"},  {1200, L"20:00"},   {1500, L"25:00"},
                {1800, L"30:00"}, {2700, L"45:00"}, {3600, L"1:00:00"},
            };
            HMENU menu = CreatePopupMenu();
            for (int i = 0; i < (int)std::size(presets); ++i) AppendMenuW(menu, MF_STRING, 1 + i, presets[i].label);
            POINT scr = pt;
            ClientToScreen(hwnd, &scr);
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, scr.x, scr.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            if (cmd > 0) {
                auto& ts = s.app.timers[idx];
                ts.dur = seconds{presets[cmd - 1].secs};
                ts.t.reset();
                ts.t.set(ts.dur);
                save_config(hwnd, s);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &pt);
        int idx = timer_index_at_y(s, pt.y);
        if (idx >= 0 && !s.app.timers[idx].t.touched()) {
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
    case WM_KEYDOWN: {
        switch (wp) {
        case VK_SPACE:
            if (s.app.show_sw)
                handle(hwnd, A_SW_START, s);
            else if (s.app.show_tmr && !s.app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_START), s);
            break;
        case 'L':
            if (s.app.show_sw) handle(hwnd, A_SW_LAP, s);
            break;
        case 'E':
            if (s.app.show_sw) handle(hwnd, A_SW_GET, s);
            break;
        case 'R':
            if (s.app.show_sw)
                handle(hwnd, A_SW_RESET, s);
            else if (s.app.show_tmr && !s.app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_RST), s);
            break;
        case 'T':
            handle(hwnd, A_TOPMOST, s);
            break;
        case 'D':
            handle(hwnd, A_THEME, s);
            break;
        case '1':
        case '2':
        case '3': {
            int idx = (int)(wp - '1');
            if (s.app.show_tmr && idx < (int)s.app.timers.size()) handle(hwnd, tmr_act(idx, A_TMR_START), s);
            break;
        }
        case 'H':
            s.app.show_help = !s.app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_OEM_PLUS:
            if (s.app.show_tmr && (int)s.app.timers.size() < Config::MAX_TIMERS)
                handle(hwnd, tmr_act((int)s.app.timers.size() - 1, A_TMR_ADD), s);
            break;
        case VK_OEM_MINUS:
            if (s.app.show_tmr && (int)s.app.timers.size() > 1)
                handle(hwnd, tmr_act((int)s.app.timers.size() - 1, A_TMR_DEL), s);
            break;
        }
        return 0;
    }
    case WM_CHAR:
        if (wp == '?') {
            s.app.show_help = !s.app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    default:
        return std::nullopt;
    }
}
