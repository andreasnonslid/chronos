module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <chrono>
export module input_core;
import actions;
import config_io;
import geometry;
import polling;
import wndstate;

using namespace std::chrono;
using sc = steady_clock;

export void handle(HWND hwnd, int act, WndState& s) {
    auto now = sc::now();
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
    if (r.apply_theme) apply_theme(hwnd, s);
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd, s);
}
