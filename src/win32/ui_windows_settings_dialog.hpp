#pragma once
#include <windows.h>
#include "app.hpp"
#include "theme.hpp"

namespace ui {
bool show_settings_dialog(HWND parent, App& app, HFONT font,
                          const Theme* theme = nullptr, int dpi = 0);
} // namespace ui
