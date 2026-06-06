#include "platform_window.hpp"

#include <SDL.h>
#include <SDL_syswm.h>
#include <shellapi.h>
#include <windows.h>

namespace {

constexpr int RESIZE_BORDER = 10;
constexpr UINT TRAY_CALLBACK_MESSAGE = WM_APP + 42;
constexpr UINT TRAY_ICON_ID = 1;

NOTIFYICONDATAW g_tray{};
bool g_tray_added = false;
WNDPROC g_prev_wndproc = nullptr;

HWND hwnd_from_sdl(SDL_Window* window) {
    if (!window) return nullptr;
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info)) return nullptr;
    return info.info.win.window;
}

void ensure_tray_icon(HWND hwnd) {
    if (!hwnd || g_tray_added) return;
    g_tray = {};
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = hwnd;
    g_tray.uID = TRAY_ICON_ID;
    g_tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_tray.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
    g_tray.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_tray.szTip, L"Chronos");
    g_tray_added = Shell_NotifyIconW(NIM_ADD, &g_tray) == TRUE;
    if (g_tray_added) {
        g_tray.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &g_tray);
    }
}

void remove_tray_icon() {
    if (!g_tray_added) return;
    Shell_NotifyIconW(NIM_DELETE, &g_tray);
    g_tray_added = false;
}

void restore_hwnd(HWND hwnd) {
    if (!hwnd) return;
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    remove_tray_icon();
}

LRESULT CALLBACK chronos_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == TRAY_CALLBACK_MESSAGE) {
        if (LOWORD(lp) == WM_LBUTTONUP || LOWORD(lp) == WM_LBUTTONDBLCLK) {
            restore_hwnd(hwnd);
        }
        return 0;
    }
    return g_prev_wndproc ? CallWindowProcW(g_prev_wndproc, hwnd, msg, wp, lp)
                          : DefWindowProcW(hwnd, msg, wp, lp);
}

SDL_HitTestResult chronos_hit_test(SDL_Window* window, const SDL_Point* area, void*) {
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (!area || w <= 0 || h <= 0) return SDL_HITTEST_NORMAL;

    const bool left = area->x >= 0 && area->x < RESIZE_BORDER;
    const bool right = area->x < w && area->x >= w - RESIZE_BORDER;
    const bool top = area->y >= 0 && area->y < RESIZE_BORDER;
    const bool bottom = area->y < h && area->y >= h - RESIZE_BORDER;

    if (top && left) return SDL_HITTEST_RESIZE_TOPLEFT;
    if (top && right) return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (bottom && left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (top) return SDL_HITTEST_RESIZE_TOP;
    if (bottom) return SDL_HITTEST_RESIZE_BOTTOM;
    if (left) return SDL_HITTEST_RESIZE_LEFT;
    if (right) return SDL_HITTEST_RESIZE_RIGHT;
    return SDL_HITTEST_NORMAL;
}

}  // namespace

void platform_window_init(SDL_Window* window) {
    if (!window) return;
    SDL_SetWindowResizable(window, SDL_TRUE);
    SDL_SetWindowHitTest(window, chronos_hit_test, nullptr);

    HWND hwnd = hwnd_from_sdl(window);
    if (!hwnd) return;
    if (!g_prev_wndproc) {
        g_prev_wndproc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)chronos_wndproc);
    }
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    style &= ~WS_CAPTION;
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void platform_window_shutdown(SDL_Window* window) {
    if (!window) return;
    SDL_SetWindowHitTest(window, nullptr, nullptr);
    HWND hwnd = hwnd_from_sdl(window);
    if (hwnd && g_prev_wndproc) {
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)g_prev_wndproc);
        g_prev_wndproc = nullptr;
    }
    remove_tray_icon();
}

void platform_set_always_on_top(SDL_Window* window, bool topmost) {
    if (!window) return;
    SDL_SetWindowAlwaysOnTop(window, topmost ? SDL_TRUE : SDL_FALSE);
    HWND hwnd = hwnd_from_sdl(window);
    if (!hwnd) return;
    SetWindowPos(hwnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    if (topmost) SDL_RaiseWindow(window);
}

void platform_begin_window_drag(SDL_Window* window) {
    HWND hwnd = hwnd_from_sdl(window);
    if (!hwnd) return;
    ReleaseCapture();
    SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

void platform_minimize_to_tray(SDL_Window* window) {
    HWND hwnd = hwnd_from_sdl(window);
    if (!hwnd) return;
    ensure_tray_icon(hwnd);
    ShowWindow(hwnd, SW_HIDE);
}

void platform_restore_from_tray(SDL_Window* window) {
    restore_hwnd(hwnd_from_sdl(window));
}

void platform_notify(SDL_Window* window, const char* title, const char* body) {
    HWND hwnd = hwnd_from_sdl(window);
    if (!hwnd) return;
    ensure_tray_icon(hwnd);
    g_tray.uFlags = NIF_INFO;
    MultiByteToWideChar(CP_UTF8, 0, title ? title : "Chronos", -1,
                        g_tray.szInfoTitle, ARRAYSIZE(g_tray.szInfoTitle));
    MultiByteToWideChar(CP_UTF8, 0, body ? body : "", -1,
                        g_tray.szInfo, ARRAYSIZE(g_tray.szInfo));
    g_tray.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &g_tray);
}

void platform_beep() {
    MessageBeep(MB_ICONASTERISK);
}
