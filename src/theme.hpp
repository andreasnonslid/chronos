#pragma once
#include <windows.h>
#include <chrono>

#include "ui_style.hpp"

constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR = 20;

constexpr COLORREF colorref_from_ui(UiColor color) { return RGB(color.r, color.g, color.b); }

constexpr UiColor ui_color_from_colorref(COLORREF color) {
    return UiColor{
        static_cast<std::uint8_t>(color & 0xFF),
        static_cast<std::uint8_t>((color >> 8) & 0xFF),
        static_cast<std::uint8_t>((color >> 16) & 0xFF),
    };
}

// ─── Theme ────────────────────────────────────────────────────────────────────
struct Theme {
    ThemePalette palette;
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

constexpr Theme make_theme(const ThemePalette& palette) {
    return Theme{
        .palette = palette,
        .bg = colorref_from_ui(palette.bg),
        .bar = colorref_from_ui(palette.bar),
        .btn = colorref_from_ui(palette.btn),
        .active = colorref_from_ui(palette.active),
        .text = colorref_from_ui(palette.text),
        .dim = colorref_from_ui(palette.dim),
        .warn = colorref_from_ui(palette.warn),
        .expire = colorref_from_ui(palette.expire),
        .blink = colorref_from_ui(palette.blink),
        .fill = colorref_from_ui(palette.fill),
        .fill_exp = colorref_from_ui(palette.fill_exp),
        .help_bg = colorref_from_ui(palette.help_bg),
        .divider = colorref_from_ui(palette.divider),
    };
}

constexpr Theme dark_theme = make_theme(dark_palette);
constexpr Theme light_theme = make_theme(light_palette);

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
