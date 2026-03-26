module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
// Forward-declare DWM to avoid MinGW header chain issues (uxtheme.h missing commctrl.h).
extern "C"
    __declspec(dllimport) HRESULT __stdcall DwmSetWindowAttribute(HWND hwnd, DWORD attr, LPCVOID data, DWORD size);
#include <algorithm>
#include <chrono>
#include <format>
#include <memory>
#include <string>
#include <vector>
export module window;
import actions;
import app;
import config;
import config_io;
import debug;
import dpi;
import formatting;
import icon;
import layout;
import painting;
import stopwatch;
import theme;
import timer;
import tray;
import wndstate;

using namespace std::chrono;
using sc = steady_clock;

// ─── Named constants ─────────────────────────────────────────────────────────
constexpr int POLL_STOPWATCH_MS = 20;
constexpr int POLL_TIMER_MS = 100;
constexpr int POLL_IDLE_MS = 1000;

// ─── Window helpers ──────────────────────────────────────────────────────────
static LayoutState layout_state(const WndState& s) {
    return {
        .show_clk = s.app.show_clk,
        .show_sw = s.app.show_sw,
        .show_tmr = s.app.show_tmr,
        .timer_count = (int)s.app.timers.size(),
    };
}

static int client_height(const WndState& s) { return client_height_for(s.layout, layout_state(s)); }

static void sync_timer(HWND hwnd, WndState& s) {
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

static int nonclient_height(HWND hwnd) {
    RECT wr, cr;
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);
    return (wr.bottom - wr.top) - cr.bottom;
}

static void resize_window(HWND hwnd, const WndState& s) {
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int cur_w = wr.right - wr.left;
    SetWindowPos(hwnd, nullptr, 0, 0, cur_w, client_height(s) + nonclient_height(hwnd), SWP_NOMOVE | SWP_NOZORDER);
}

// ─── Timer hit detection ─────────────────────────────────────────────────────
static int timer_index_at_y(const WndState& s, int y) {
    if (!s.app.show_tmr) return -1;
    int top = s.layout.bar_h;
    if (s.app.show_clk) top += s.layout.clk_h;
    if (s.app.show_sw) top += s.layout.sw_h;
    if (y < top) return -1;
    int idx = (y - top) / s.layout.tmr_h;
    return idx < (int)s.app.timers.size() ? idx : -1;
}

// ─── Action wrapper ──────────────────────────────────────────────────────────
static void handle(HWND hwnd, int act, WndState& s) {
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
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd, s);
}

// ─── WndProc ──────────────────────────────────────────────────────────────────
export LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* s = (WndState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        auto state = std::make_unique<WndState>();
        s = state.get();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)s);
        auto* cs = (CREATESTRUCTW*)lp;
        if (cs->lpCreateParams) s->layout = *(Layout*)cs->lpCreateParams;
        if (pGetDpiForWindow) {
            UINT wdpi = pGetDpiForWindow(hwnd);
            if (wdpi != 0) s->layout.update_for_dpi((int)wdpi);
        }
        recreate_fonts(*s);
        bool dark = system_prefers_dark();
        s->active_theme = dark ? &dark_theme : &light_theme;
        s->create_brushes();
        SetTimer(hwnd, 1, POLL_TIMER_MS, nullptr);
        BOOL dwm_dark = dark ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR, &dwm_dark, sizeof(dwm_dark));
        s->tray_icon = create_app_icon(16);
        s->cfg_path = config_path();
        load_config(hwnd, *s);
        resize_window(hwnd, *s);
        state.release();
        return 0;
    }
    default:
        break;
    }

    if (!s) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_TIMER: {
        auto now = sc::now();
        for (auto& ts : s->app.timers) {
            if (ts.t.touched() && ts.t.expired(now) && !ts.notified) {
                ts.notified = true;
                MessageBeep(MB_ICONASTERISK);
                FLASHWINFO fi{};
                fi.cbSize = sizeof(fi);
                fi.hwnd = hwnd;
                fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
                fi.uCount = 3;
                fi.dwTimeout = 0;
                FlashWindowEx(&fi);
                if (s->tray_active) {
                    auto lbl = ts.label.empty() ? L"Timer" : ts.label.c_str();
                    tray_notify(hwnd, L"Timer expired", lbl);
                }
            }
        }
        {
            std::wstring title;
            for (auto& ts : s->app.timers) {
                if (ts.t.is_running()) {
                    title = format_timer_display(ts.t.remaining(now));
                    if (ts.t.expired(now)) title = L"EXPIRED " + title;
                    break;
                }
            }
            if (title.empty() && s->app.sw.is_running()) {
                title = format_stopwatch_display(s->app.sw.elapsed(now));
            }
            if (title.empty()) {
                SYSTEMTIME st;
                GetLocalTime(&st);
                title = std::format(L"{:02}:{:02}:{:02}", st.wHour, st.wMinute, st.wSecond);
            }
            title += L" \u2014 Chronos";
            SetWindowTextW(hwnd, title.c_str());
            if (s->tray_active) tray_update_tip(hwnd, title.c_str());
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        sync_timer(hwnd, *s);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        if (wp == SIZE_MINIMIZED) {
            tray_add(hwnd, s->tray_icon);
            s->tray_active = true;
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        break;
    case WM_TRAYICON:
        if (lp == WM_LBUTTONUP) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
            tray_remove(hwnd);
            s->tray_active = false;
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
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
                tray_remove(hwnd);
                s->tray_active = false;
            } else if (cmd == IDM_TRAY_EXIT) {
                tray_remove(hwnd);
                s->tray_active = false;
                DestroyWindow(hwnd);
            }
        }
        return 0;
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
    case WM_LBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        for (auto& [r, id] : s->btns)
            if (PtInRect(&r, pt)) {
                handle(hwnd, id, *s);
                break;
            }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        for (auto& [r, id] : s->btns)
            if (PtInRect(&r, pt)) {
                handle(hwnd, id, *s);
                return 0;
            }
        int idx = timer_index_at_y(*s, pt.y);
        if (idx >= 0) {
            auto& ts = s->app.timers[idx];
            wchar_t buf[21] = {};
            if (!ts.label.empty()) wcsncpy(buf, ts.label.c_str(), 20);
            RECT dlg_r;
            GetClientRect(hwnd, &dlg_r);
            int top = s->layout.bar_h;
            if (s->app.show_clk) top += s->layout.clk_h;
            if (s->app.show_sw) top += s->layout.sw_h;
            int ey = top + idx * s->layout.tmr_h + s->layout.dpi_scale(2);
            int eh = s->layout.dpi_scale(18);
            int ew = s->layout.dpi_scale(120);
            int ex = (dlg_r.right - ew) / 2;
            HWND edit = CreateWindowExW(0, L"EDIT", buf, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
                                        ex, ey, ew, eh, hwnd, (HMENU)(INT_PTR)(9000 + idx), nullptr, nullptr);
            SendMessageW(edit, EM_SETLIMITTEXT, 20, 0);
            SendMessageW(edit, WM_SETFONT, (WPARAM)s->hFontSm, TRUE);
            SetFocus(edit);
            SendMessageW(edit, EM_SETSEL, 0, -1);
        }
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp);
        int code = HIWORD(wp);
        if (id >= 9000 && id < 9000 + Config::MAX_TIMERS && code == EN_KILLFOCUS) {
            int idx = id - 9000;
            HWND edit = (HWND)lp;
            wchar_t buf[21] = {};
            GetWindowTextW(edit, buf, 21);
            if (idx < (int)s->app.timers.size()) {
                s->app.timers[idx].label = buf;
                save_config(hwnd, *s);
            }
            DestroyWindow(edit);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_RBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        int idx = timer_index_at_y(*s, pt.y);
        if (idx >= 0 && !s->app.timers[idx].t.touched()) {
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
                auto& ts = s->app.timers[idx];
                ts.dur = seconds{presets[cmd - 1].secs};
                ts.t.reset();
                ts.t.set(ts.dur);
                save_config(hwnd, *s);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &pt);
        int idx = timer_index_at_y(*s, pt.y);
        if (idx >= 0 && !s->app.timers[idx].t.touched()) {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            bool up = delta > 0;
            int off;
            if (GetKeyState(VK_CONTROL) & 0x8000)
                off = up ? A_TMR_HUP : A_TMR_HDN;
            else if (GetKeyState(VK_SHIFT) & 0x8000)
                off = up ? A_TMR_SUP : A_TMR_SDN;
            else
                off = up ? A_TMR_MUP : A_TMR_MDN;
            handle(hwnd, tmr_act(idx, off), *s);
        }
        return 0;
    }
    case WM_KEYDOWN: {
        switch (wp) {
        case VK_SPACE:
            if (s->app.show_sw)
                handle(hwnd, A_SW_START, *s);
            else if (s->app.show_tmr && !s->app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_START), *s);
            break;
        case 'L':
            if (s->app.show_sw) handle(hwnd, A_SW_LAP, *s);
            break;
        case 'E':
            if (s->app.show_sw) handle(hwnd, A_SW_GET, *s);
            break;
        case 'R':
            if (s->app.show_sw)
                handle(hwnd, A_SW_RESET, *s);
            else if (s->app.show_tmr && !s->app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_RST), *s);
            break;
        case 'T':
            handle(hwnd, A_TOPMOST, *s);
            break;
        case '1':
        case '2':
        case '3': {
            int idx = (int)(wp - '1');
            if (s->app.show_tmr && idx < (int)s->app.timers.size()) handle(hwnd, tmr_act(idx, A_TMR_START), *s);
            break;
        }
        case 'H':
            s->app.show_help = !s->app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
        return 0;
    }
    case WM_CHAR:
        if (wp == '?') {
            s->app.show_help = !s->app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
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
        if (lp && wcscmp((LPCWSTR)lp, L"ImmersiveColorSet") == 0) {
            bool dark = system_prefers_dark();
            BOOL dwm_dark = dark ? TRUE : FALSE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR, &dwm_dark, sizeof(dwm_dark));
            s->active_theme = dark ? &dark_theme : &light_theme;
            s->destroy_brushes();
            s->create_brushes();
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    case WM_EXITSIZEMOVE:
        save_config(hwnd, *s);
        return 0;
    case WM_DESTROY: {
        if (s->tray_active) tray_remove(hwnd);
        if (s->tray_icon) DestroyIcon(s->tray_icon);
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
