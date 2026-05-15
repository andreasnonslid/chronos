#pragma once
#include <windows.h>
#include <optional>
#include "wndstate.hpp"

constexpr int HOTKEY_GLOBAL = 1;

std::optional<LRESULT> dispatch_keyboard(HWND hwnd, UINT msg, WPARAM wp, WndState& s);
