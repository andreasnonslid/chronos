#pragma once
#include <windows.h>
#include <optional>
#include "wndstate.hpp"

std::optional<LRESULT> dispatch_mouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, WndState& s);
