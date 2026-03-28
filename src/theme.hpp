#pragma once
#include <windows.h>
#include <chrono>

constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR = 20;

// ─── Theme ────────────────────────────────────────────────────────────────────
struct Theme {
    COLORREF bg;
    COLORREF bar;
    COLORREF btn;
    COLORREF active;
    COLORREF text;
    COLORREF dim;
    COLORREF warn;
    COLORREF expire;
    COLORREF blink;
    COLORREF fill;
    COLORREF fill_exp;
    COLORREF help_bg;
    COLORREF divider;
};

constexpr Theme dark_theme{
    .bg = RGB(26, 26, 26),
    .bar = RGB(35, 35, 38),
    .btn = RGB(40, 40, 44),
    .active = RGB(80, 80, 88),
    .text = RGB(204, 204, 204),
    .dim = RGB(90, 90, 90),
    .warn = RGB(240, 140, 30),
    .expire = RGB(200, 50, 50),
    .blink = RGB(110, 110, 118),
    .fill = RGB(38, 38, 50),
    .fill_exp = RGB(72, 18, 18),
    .help_bg = RGB(20, 20, 20),
    .divider = RGB(50, 50, 55),
};

constexpr Theme light_theme{
    .bg = RGB(243, 243, 243),
    .bar = RGB(228, 228, 232),
    .btn = RGB(214, 214, 220),
    .active = RGB(175, 175, 188),
    .text = RGB(28, 28, 28),
    .dim = RGB(138, 138, 138),
    .warn = RGB(196, 90, 0),
    .expire = RGB(196, 36, 36),
    .blink = RGB(168, 168, 178),
    .fill = RGB(208, 208, 230),
    .fill_exp = RGB(240, 196, 196),
    .help_bg = RGB(232, 232, 232),
    .divider = RGB(198, 198, 208),
};

// ─── Blink duration ──────────────────────────────────────────────────────────
constexpr auto BLINK_DUR = std::chrono::milliseconds{120};

inline bool system_prefers_dark() {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                      KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD val = 0, size = sizeof(val);
        bool ok = RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&val, &size) == ERROR_SUCCESS;
        RegCloseKey(key);
        if (ok) return val == 0;
    }
    return true;
}
