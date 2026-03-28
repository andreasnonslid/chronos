module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <optional>
export module input;
import input_keyboard;
import input_mouse;
import wndstate;

export std::optional<LRESULT> dispatch_input(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, WndState& s) {
    if (auto r = dispatch_keyboard(hwnd, msg, wp, s); r) return r;
    if (auto r = dispatch_mouse(hwnd, msg, wp, lp, s); r) return r;
    return std::nullopt;
}
