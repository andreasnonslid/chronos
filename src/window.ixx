module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
// Forward-declare DWM to avoid MinGW header chain issues (uxtheme.h missing commctrl.h).
extern "C"
    __declspec(dllimport) HRESULT __stdcall DwmSetWindowAttribute(HWND hwnd, DWORD attr, LPCVOID data, DWORD size);
#include <memory>
export module window;
import config_io;
import dpi;
import icon;
import input;
import layout;
import painting;
import polling;
import theme;
import tray;
import wndstate;

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
