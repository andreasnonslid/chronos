#pragma once
#include <windows.h>
#include "app.hpp"
#include "theme.hpp"

bool show_pomodoro_interval_dialog(HWND parent, App& app, HFONT font,
                                   const Theme* theme, int dpi);
