#include "input_core.hpp"
#include <windows.h>
#include <shellapi.h>
#include <chrono>
#include "actions.hpp"
#include "alarm_dialog.hpp"
#include "config_io.hpp"
#include "geometry.hpp"
#include "polling.hpp"
#include "ui.hpp"
#include "wndstate.hpp"

void copy_laps_to_clipboard(HWND hwnd, const Stopwatch& sw) {
    const auto& laps = sw.laps();
    std::wstring text;
    Stopwatch::dur cumulative{};
    for (size_t i = 0; i < laps.size(); ++i) {
        cumulative += laps[i];
        text += format_lap_row(i + 1, laps[i], cumulative);
        text += L'\n';
    }
    if (text.empty()) return;
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hg) return;
    auto* dest = (wchar_t*)GlobalLock(hg);
    if (dest) {
        memcpy(dest, text.c_str(), bytes);
        GlobalUnlock(hg);
        if (OpenClipboard(hwnd)) {
            EmptyClipboard();
            SetClipboardData(CF_UNICODETEXT, hg);
            CloseClipboard();
        } else {
            GlobalFree(hg);
        }
    } else {
        GlobalFree(hg);
    }
}

void handle(HWND hwnd, int act, WndState& s) {
    auto now = std::chrono::steady_clock::now();
    if (wants_blink(act)) {
        s.app.blink_act = act;
        s.app.blink_t = now;
    }
    auto r = dispatch_action(s.app, act, now, s.cfg_path.parent_path());
    if (r.set_topmost)
        SetWindowPos(hwnd, s.app.topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if (r.resize) resize_window(hwnd, s);
    if (r.save_config) save_config(hwnd, s);
    if (r.open_file) ShellExecuteW(nullptr, L"open", s.app.sw_lap_file.c_str(), nullptr, nullptr, SW_SHOW);
    if (r.copy_laps) {
        copy_laps_to_clipboard(hwnd, s.app.sw);
        s.clipboard_copied_until = now + std::chrono::seconds{1};
    }
    if (r.apply_theme) apply_theme(hwnd, s);
    if (r.open_settings) {
        auto old_theme = s.app.theme_mode;
        auto old_clock = s.app.clock_view;
        if (ui::show_settings_dialog(hwnd, s.app, (ui::FontHandle)s.fontSm.h, s.active_theme, s.layout.dpi)) {
            if (s.app.theme_mode != old_theme) apply_theme(hwnd, s);
            if (s.app.clock_view != old_clock) resize_window(hwnd, s);
            save_config(hwnd, s);
        }
    }
    if (r.open_alarm_dialog && s.app.alarms.size() < (size_t)ALARM_MAX_COUNT) {
        Alarm new_alarm;
        if (alarm_dlg::show_add_alarm_dialog(hwnd, new_alarm,
                (HFONT)s.fontSm.h, s.active_theme, s.layout.dpi)) {
            s.app.alarms.push_back(new_alarm);
            resize_window(hwnd, s);
            save_config(hwnd, s);
        }
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd, s);
}
