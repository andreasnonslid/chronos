#include "platform_window.hpp"

#include <SDL.h>
#include <SDL_syswm.h>
#include <windows.h>

namespace {

constexpr int RESIZE_BORDER = 10;

HWND hwnd_from_sdl(SDL_Window* window) {
    if (!window) return nullptr;
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info)) return nullptr;
    return info.info.win.window;
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
