#pragma once
#include <windows.h>
#include <shellapi.h>
#include <memory>
#include "dwm_fwd.hpp"
#include "config.hpp"
#include "config_io.hpp"
#include "dpi.hpp"
#include "geometry.hpp"
#include "gdi.hpp"
#include "icon.hpp"
#include "input.hpp"
#include "layout.hpp"
#include "painting.hpp"
#include "polling.hpp"
#include "theme.hpp"
#include "tray.hpp"
#include "wndstate.hpp"

// ─── WndProc ────────────────────────────────────────────────────────────────────
inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* s = (WndState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        auto state = std::make_unique<WndState>();
        s = state.get();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)s);
        auto* cs = (CREATESTRUCTW*)lp;
        if (cs->lpCreateParams) s->layout = *(Layout*)cs->lpCreateParams;
        if (g_dpi.GetDpiForWindow) {
            UINT wdpi = g_dpi.GetDpiForWindow(hwnd);
            if (wdpi != 0) s->layout.update_for_dpi((int)wdpi);
        }
        recreate_fonts(*s);
        SetTimer(hwnd, 1, POLL_TIMER_MS, nullptr);
        s->cfg_path = config_path();
        load_config(hwnd, *s);
        apply_theme(hwnd, *s);
        resize_window(hwnd, *s);
        s->global_hotkey_ok = RegisterHotKey(hwnd, HOTKEY_GLOBAL,
                                               MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, VK_SPACE) != 0;
        if (!s->global_hotkey_ok)
            dbg(L"RegisterHotKey failed for Ctrl+Shift+Space (already claimed by another app?)");
        state.release();
        return 0;
    }
    default:
        break;
    }

    if (!s) return DefWindowProcW(hwnd, msg, wp, lp);

    if (auto r = dispatch_input(hwnd, msg, wp, lp, *s); r.has_value())
        return *r;

    switch (msg) {
    case WM_TIMER:
        handle_wm_timer(hwnd, *s);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        if (wp == SIZE_MINIMIZED) {
            tray_add(hwnd, s->tray_icon);
            s->tray_active = true;
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        {
            RECT cr;
            GetClientRect(hwnd, &cr);
            if (cr.bottom != client_height(*s))
                resize_window(hwnd, *s);
        }
        break;
    case WM_TRAYICON: {
        auto tray_restore = [&] {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
            tray_remove(hwnd);
            s->tray_active = false;
        };
        if (lp == WM_LBUTTONUP) {
            tray_restore();
        } else if (lp == WM_RBUTTONUP) {
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, IDM_TRAY_SHOW, L"Show");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, IDM_TRAY_EXIT, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            if (cmd == IDM_TRAY_SHOW) {
                tray_restore();
            } else if (cmd == IDM_TRAY_EXIT) {
                tray_remove(hwnd);
                s->tray_active = false;
                DestroyWindow(hwnd);
            }
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT cr;
        GetClientRect(hwnd, &cr);
        s->ensure_buffer(hdc, cr.right, cr.bottom);
        auto ctx = s->paint_ctx();
        paint_all(s->mdc, cr.right, cr.bottom, ctx);
        BitBlt(hdc, 0, 0, cr.right, cr.bottom, s->mdc, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SIZING: {
        int want_h = client_height(*s) + nonclient_height(hwnd);
        auto* r = (RECT*)lp;
        if (wp == WMSZ_TOP || wp == WMSZ_TOPLEFT || wp == WMSZ_TOPRIGHT)
            r->top = r->bottom - want_h;
        else
            r->bottom = r->top + want_h;
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
        auto* m = (MINMAXINFO*)lp;
        DWORD ws = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
        RECT adj{0, 0, s->layout.bar_min_client_w(), 0};
        AdjustWindowRectEx(&adj, ws, FALSE, 0);
        m->ptMinTrackSize.x = adj.right - adj.left;
        RECT adj_h{0, 0, 0, client_height(*s)};
        AdjustWindowRectEx(&adj_h, ws, FALSE, 0);
        m->ptMinTrackSize.y = adj_h.bottom - adj_h.top;
        m->ptMaxTrackSize.y = m->ptMinTrackSize.y;
        m->ptMaxSize.y = m->ptMinTrackSize.y;
        return 0;
    }
    case WM_WINDOWPOSCHANGED: {
        RECT cr;
        GetClientRect(hwnd, &cr);
        if (cr.bottom != client_height(*s))
            resize_window(hwnd, *s);
        return 0;
    }
    case WM_DPICHANGED: {
        s->layout.update_for_dpi(HIWORD(wp));
        recreate_fonts(*s);
        auto* suggested = (RECT*)lp;
        SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                     suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
        resize_window(hwnd, *s);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_SETTINGCHANGE:
        if (lp && wcscmp((LPCWSTR)lp, L"ImmersiveColorSet") == 0 &&
                s->app.theme_mode == ThemeMode::Auto) {
            apply_theme(hwnd, *s);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    case WM_EXITSIZEMOVE:
        save_config(hwnd, *s);
        return 0;
    case WM_DESTROY: {
        UnregisterHotKey(hwnd, HOTKEY_GLOBAL);
        if (s->tray_active) tray_remove(hwnd);
        save_config(hwnd, *s);
        KillTimer(hwnd, 1);
        auto owned = std::unique_ptr<WndState>(s);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
