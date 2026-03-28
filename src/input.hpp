#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <optional>
#include "input_keyboard.hpp"
#include "input_mouse.hpp"
#include "wndstate.hpp"

inline std::optional<LRESULT> dispatch_input(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, WndState& s) {
    if (auto r = dispatch_keyboard(hwnd, msg, wp, s); r) return r;
    if (auto r = dispatch_mouse(hwnd, msg, wp, lp, s); r) return r;
    return std::nullopt;
}
