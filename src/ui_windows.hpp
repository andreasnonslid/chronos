#pragma once
#include <windows.h>
#include "theme.hpp"
#include "ui_windows_settings_dialog.hpp"

namespace ui {
using WindowHandle   = HWND;
using FontHandle     = HFONT;
using ThemeHandle    = const Theme*;
using InstanceHandle = HINSTANCE;

int run_app(InstanceHandle hInst, int nShow);
} // namespace ui
