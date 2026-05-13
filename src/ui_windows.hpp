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

namespace ui {
using WindowHandle = HWND;
using FontHandle = HFONT;
using ThemeHandle = const Theme*;
using InstanceHandle = HINSTANCE;

namespace settings_dlg {
#include "ui_windows_settings_dialog_ids.hpp"
#include "ui_windows_settings_dialog_template.hpp"
#include "ui_windows_settings_dialog_state.hpp"
#include "ui_windows_settings_dialog_scroll.hpp"
#include "ui_windows_settings_dialog_paint.hpp"
#include "ui_windows_settings_dialog_handlers.hpp"
} // namespace settings_dlg

#include "ui_windows_settings_dialog_show.hpp"
} // namespace ui

#include "debug.hpp"
#include "dpi.hpp"
#include "icon.hpp"
#include "layout.hpp"
#include "window.hpp"

namespace ui {
#include "ui_windows_app.hpp"
} // namespace ui
