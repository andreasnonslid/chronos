#pragma once
#include <windows.h>
#include <optional>
#include "actions.hpp"
#include "config.hpp"
#include "input_core.hpp"
#include "wndstate.hpp"

inline std::optional<LRESULT> dispatch_keyboard(HWND hwnd, UINT msg, WPARAM wp, WndState& s) {
    switch (msg) {
    case WM_KEYDOWN:
        switch (wp) {
        case VK_SPACE:
            if (s.app.show_sw)
                handle(hwnd, A_SW_START, s);
            else if (s.app.show_tmr && !s.app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_START), s);
            return 0;
        case 'L':
            if (s.app.show_sw) handle(hwnd, A_SW_LAP, s);
            return 0;
        case 'E':
            if (s.app.show_sw) handle(hwnd, A_SW_GET, s);
            return 0;
        case 'R':
            if (s.app.show_sw)
                handle(hwnd, A_SW_RESET, s);
            else if (s.app.show_tmr && !s.app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_RST), s);
            return 0;
        case 'T':
            handle(hwnd, A_TOPMOST, s);
            return 0;
        case 'D':
            handle(hwnd, A_THEME, s);
            return 0;
        case '1':
        case '2':
        case '3': {
            int idx = (int)(wp - '1');
            if (s.app.show_tmr && idx < (int)s.app.timers.size()) handle(hwnd, tmr_act(idx, A_TMR_START), s);
            return 0;
        }
        case 'H':
            s.app.show_help = !s.app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case VK_OEM_PLUS:
            if (s.app.show_tmr && (int)s.app.timers.size() < Config::MAX_TIMERS)
                handle(hwnd, tmr_act((int)s.app.timers.size() - 1, A_TMR_ADD), s);
            return 0;
        case VK_OEM_MINUS:
            if (s.app.show_tmr && (int)s.app.timers.size() > 1)
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
