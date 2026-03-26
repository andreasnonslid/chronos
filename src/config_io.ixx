module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
export module config_io;
import config;
import debug;
import layout;
import wndstate;

using namespace std::chrono;

static std::string wide_to_utf8(const std::wstring& w) {
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), len, nullptr, nullptr);
    return s;
}

static std::wstring utf8_to_wide(const std::string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    return w;
}

export std::filesystem::path config_path() {
    if (auto* appdata = _wgetenv(L"APPDATA")) {
        auto dir = std::filesystem::path{appdata} / L"Chronos";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (!ec) return dir / L"config.ini";
    }
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    return std::filesystem::path{exe}.parent_path() / "config.ini";
}

export void save_config(HWND hwnd, const WndState& s) {
    const auto& path = s.cfg_path;
    dbg(std::format(L"[chrono] save_config: {}", path.wstring()));
    std::ofstream f(path);
    if (!f) {
        dbg(L"[chrono] save_config: open failed");
        return;
    }
    Config cfg;
    cfg.show_clk = s.app.show_clk;
    cfg.show_sw = s.app.show_sw;
    cfg.show_tmr = s.app.show_tmr;
    cfg.topmost = s.app.topmost;
    cfg.num_timers = (int)s.app.timers.size();
    for (int i = 0; i < (int)s.app.timers.size(); ++i) {
        cfg.timer_secs[i] = (int)s.app.timers[i].dur.count();
        cfg.timer_labels[i] = wide_to_utf8(s.app.timers[i].label);
    }
    if (hwnd) {
        RECT wr;
        if (IsIconic(hwnd)) {
            WINDOWPLACEMENT wp{};
            wp.length = sizeof(wp);
            GetWindowPlacement(hwnd, &wp);
            wr = wp.rcNormalPosition;
        } else {
            GetWindowRect(hwnd, &wr);
        }
        cfg.pos_valid = true;
        cfg.win_x = wr.left;
        cfg.win_y = wr.top;
        cfg.win_w = wr.right - wr.left;
    }
    config_write(cfg, f);
}

export void load_config(HWND hwnd, WndState& s) {
    const auto& path = s.cfg_path;
    dbg(std::format(L"[chrono] load_config: {}", path.wstring()));
    std::ifstream f(path);
    if (!f) {
        dbg(L"[chrono] no config, using defaults");
        return;
    }
    Config cfg;
    config_read(cfg, f);
    dbg(std::format(L"[chrono] loaded: clk={} sw={} tmr={} top={} pos={} ({},{} w={})", cfg.show_clk, cfg.show_sw,
                    cfg.show_tmr, cfg.topmost, cfg.pos_valid, cfg.win_x, cfg.win_y, cfg.win_w));
    s.app.show_clk = cfg.show_clk;
    s.app.show_sw = cfg.show_sw;
    s.app.show_tmr = cfg.show_tmr;
    s.app.topmost = cfg.topmost;
    int n = std::min(cfg.num_timers, Config::MAX_TIMERS);
    s.app.timers.resize(n);
    for (int i = 0; i < n; ++i) {
        s.app.timers[i].dur = seconds{cfg.timer_secs[i]};
        s.app.timers[i].t.reset();
        s.app.timers[i].t.set(s.app.timers[i].dur);
        s.app.timers[i].label = utf8_to_wide(cfg.timer_labels[i]);
    }
    if (s.app.topmost) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if (cfg.pos_valid) {
        RECT cur;
        GetWindowRect(hwnd, &cur);
        DWORD ws = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
        RECT adj{0, 0, s.layout.bar_min_client_w(), 0};
        AdjustWindowRectEx(&adj, ws, FALSE, 0);
        int min_w = adj.right - adj.left;
        int w = cfg.win_w < min_w ? min_w : cfg.win_w;
        RECT test{cfg.win_x, cfg.win_y, cfg.win_x + w, cfg.win_y + 1};
        HMONITOR hmon = MonitorFromRect(&test, MONITOR_DEFAULTTONULL);
        if (hmon) {
            SetWindowPos(hwnd, nullptr, cfg.win_x, cfg.win_y, w, cur.bottom - cur.top, SWP_NOZORDER);
        } else {
            SetWindowPos(hwnd, nullptr, 0, 0, w, cur.bottom - cur.top, SWP_NOZORDER | SWP_NOMOVE);
        }
    }
}
