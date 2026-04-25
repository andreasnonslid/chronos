#pragma once
#include <windows.h>
#include <optional>
#include "actions.hpp"
#include "config.hpp"
#include "input_core.hpp"
#include "wndstate.hpp"

constexpr int HOTKEY_GLOBAL = 1;

inline bool plain_key() {
    return GetKeyState(VK_CONTROL) >= 0
        && GetKeyState(VK_MENU) >= 0
        && GetKeyState(VK_LWIN) >= 0
        && GetKeyState(VK_RWIN) >= 0;
}

inline std::optional<LRESULT> dispatch_keyboard(HWND hwnd, UINT msg, WPARAM wp, WndState& s) {
    switch (msg) {
    case WM_HOTKEY:
        if (wp == HOTKEY_GLOBAL) {
            if (s.app.show_sw)
                handle(hwnd, A_SW_START, s);
            else if (s.app.show_tmr && !s.app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_START), s);
        }
        return 0;
    case WM_KEYDOWN:
        switch (wp) {
        case VK_SPACE:
            if (s.app.show_sw)
                handle(hwnd, A_SW_START, s);
            else if (s.app.show_tmr && !s.app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_START), s);
            return 0;
        case 'L':
            if (plain_key() && s.app.show_sw) handle(hwnd, A_SW_LAP, s);
            return 0;
        case 'E':
            if (plain_key() && s.app.show_sw) handle(hwnd, A_SW_GET, s);
            return 0;
        case 'C':
            if (plain_key() && s.app.show_sw) handle(hwnd, A_SW_COPY, s);
            return 0;
        case 'R': {
            // R alone: stopwatch reset (when shown) or first-timer reset.
            // Shift+R / Ctrl+R: reset ALL timers in one action so users
            // with multiple slots don't have to press Shift+1/2/3
            // separately. Ctrl+R is the always-available fallback (works
            // even when the stopwatch view is taking R alone for
            // stopwatch reset).
            //
            // Order matters: plain_key() does NOT check Shift, so we
            // must test the modifier branches BEFORE the plain-R path,
            // otherwise Shift+R falls into plain-R and silently resets
            // only the first timer.
            const bool ctrl = GetKeyState(VK_CONTROL) < 0;
            const bool shift = GetKeyState(VK_SHIFT) < 0;
            const bool alt = GetKeyState(VK_MENU) < 0;
            const bool winkey =
                GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0;
            if (!alt && !winkey && (ctrl ^ shift)) {
                // Exactly one of Ctrl or Shift is held (no Alt/Win).
                if (s.app.show_tmr && !s.app.timers.empty())
                    handle(hwnd, A_TMR_RST_ALL, s);
            } else if (plain_key() && !shift) {
                // Truly plain R (no modifiers at all).
                if (s.app.show_sw)
                    handle(hwnd, A_SW_RESET, s);
                else if (s.app.show_tmr && !s.app.timers.empty())
                    handle(hwnd, tmr_act(0, A_TMR_RST), s);
            }
            return 0;
        }
        case 'T':
            if (plain_key()) handle(hwnd, A_TOPMOST, s);
            return 0;
        case 'D':
            if (plain_key()) handle(hwnd, A_THEME, s);
            return 0;
        case '1':
        case '2':
        case '3': {
            if (!plain_key()) return 0;
            int idx = (int)(wp - '1');
            if (s.app.show_tmr && idx < (int)s.app.timers.size()) {
                int act = (GetKeyState(VK_SHIFT) < 0) ? A_TMR_RST : A_TMR_START;
                handle(hwnd, tmr_act(idx, act), s);
            }
            return 0;
        }
        case 'P':
            if (plain_key() && s.app.show_tmr && !s.app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_POMO), s);
            return 0;
        case 'H':
            if (plain_key()) {
                s.app.show_help = !s.app.show_help;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case VK_OEM_PLUS:
            if (plain_key() && s.app.show_tmr && (int)s.app.timers.size() < Config::MAX_TIMERS)
                handle(hwnd, tmr_act((int)s.app.timers.size() - 1, A_TMR_ADD), s);
            return 0;
        case VK_OEM_MINUS:
            if (plain_key() && s.app.show_tmr && (int)s.app.timers.size() > 1)
                handle(hwnd, tmr_act((int)s.app.timers.size() - 1, A_TMR_DEL), s);
            return 0;
        default:
            return std::nullopt;
        }
    case WM_CHAR:
        if (wp == '?') {
            s.app.show_help = !s.app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        return std::nullopt;
    default:
        return std::nullopt;
    }
}
