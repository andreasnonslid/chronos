#pragma once
#include <windows.h>
#include "alarm.hpp"
#include "theme.hpp"

namespace alarm_dlg {
bool show_add_alarm_dialog(HWND parent, Alarm& result, HFONT font,
                           const Theme* theme, int dpi);
} // namespace alarm_dlg
