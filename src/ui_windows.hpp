#pragma once
#include <windows.h>
#include <windowsx.h>
#ifndef WM_CTLCOLORCOMBOBOX
#define WM_CTLCOLORCOMBOBOX 0x0109
#endif
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <cwchar>
#include <format>
#include <vector>

#include "app.hpp"
#include "dialog_style.hpp"
#include "painting_analog.hpp"
#include "ui_windows_settings_dialog.hpp"

#include "debug.hpp"
#include "dpi.hpp"
#include "icon.hpp"
#include "layout.hpp"
#include "window.hpp"

namespace ui {
using WindowHandle = HWND;
using FontHandle = HFONT;
using ThemeHandle = const Theme*;
using InstanceHandle = HINSTANCE;
#include "ui_windows_app.hpp"
} // namespace ui
