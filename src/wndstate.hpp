#pragma once
#include <windows.h>
#include <filesystem>
#include <utility>
#include <vector>
#include "dwm_fwd.hpp"
#include "app.hpp"
#include "config.hpp"
#include "gdi.hpp"
#include "icon.hpp"
#include "layout.hpp"
#include "paint_ctx.hpp"
#include "theme.hpp"
#include "tray.hpp"

inline HFONT make_font(int pt, bool bold, const Layout& layout) {
    int h = -MulDiv(pt, layout.dpi, 72);
    return CreateFontW(h, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

struct WndState {
    App app;
    Layout layout;
    std::filesystem::path cfg_path;
    const Theme* active_theme = &dark_theme;
    std::vector<std::pair<RECT, int>> btns;
    GdiObj fontBig{nullptr}, fontLarge{nullptr}, fontSm{nullptr};
    WndResources res{
        .fontBig  = (HFONT)GetStockObject(DEFAULT_GUI_FONT),
        .fontLarge = (HFONT)GetStockObject(DEFAULT_GUI_FONT),
        .fontSm   = (HFONT)GetStockObject(DEFAULT_GUI_FONT),
    };
    int timer_ms = 100;
    bool tray_active = false;
    IconObj tray_icon{};

    GdiObj brBg, brBar, brBtn, brActive, brBlink, brFill, brFillExp, brHelp, pnNull, pnDivider;

    void create_brushes() {
        auto& th = *active_theme;
        brBg      = GdiObj{CreateSolidBrush(th.bg)};
        brBar     = GdiObj{CreateSolidBrush(th.bar)};
        brBtn     = GdiObj{CreateSolidBrush(th.btn)};
        brActive  = GdiObj{CreateSolidBrush(th.active)};
        brBlink   = GdiObj{CreateSolidBrush(th.blink)};
        brFill    = GdiObj{CreateSolidBrush(th.fill)};
        brFillExp = GdiObj{CreateSolidBrush(th.fill_exp)};
        brHelp    = GdiObj{CreateSolidBrush(th.help_bg)};
        pnNull    = GdiObj{CreatePen(PS_NULL, 0, 0)};
        pnDivider = GdiObj{CreatePen(PS_SOLID, 1, th.divider)};
        res.brBg      = (HBRUSH)brBg.h;
        res.brBar     = (HBRUSH)brBar.h;
        res.brBtn     = (HBRUSH)brBtn.h;
        res.brActive  = (HBRUSH)brActive.h;
        res.brBlink   = (HBRUSH)brBlink.h;
        res.brFill    = (HBRUSH)brFill.h;
        res.brFillExp = (HBRUSH)brFillExp.h;
        res.brHelp    = (HBRUSH)brHelp.h;
        res.pnNull    = (HPEN)pnNull.h;
        res.pnDivider = (HPEN)pnDivider.h;
    }

    WndState(const WndState&) = delete;
    WndState& operator=(const WndState&) = delete;
    WndState() = default;

    DcObj mdc{};
    GdiObj buf_bmp{};
    int buf_w = 0, buf_h = 0;

    void ensure_buffer(HDC hdc, int w, int h) {
        if (w != buf_w || h != buf_h) {
            DcObj new_mdc{CreateCompatibleDC(hdc)};
            if (!new_mdc.h) return;
            GdiObj new_bmp{CreateCompatibleBitmap(hdc, w, h)};
            if (!new_bmp.h) return;
            mdc = std::move(new_mdc);
            buf_bmp = std::move(new_bmp);
            SelectObject(mdc, buf_bmp);
            buf_w = w;
            buf_h = h;
        }
    }

    PaintCtx paint_ctx() {
        return {.app = app, .layout = layout, .theme = *active_theme, .res = res, .btns = btns, .now = {}};
    }
};

inline constexpr int FONT_PT_BIG = 26;
inline constexpr int FONT_PT_LARGE = 34;
inline constexpr int FONT_PT_SM = 11;

inline void recreate_fonts(WndState& s) {
    GdiObj newBig{make_font(FONT_PT_BIG, true, s.layout)};
    GdiObj newLarge{make_font(FONT_PT_LARGE, true, s.layout)};
    GdiObj newSm{make_font(FONT_PT_SM, false, s.layout)};
    if (!newBig.h || !newLarge.h || !newSm.h) return;
    s.fontBig = std::move(newBig);
    s.fontLarge = std::move(newLarge);
    s.fontSm = std::move(newSm);
    s.res.fontBig   = (HFONT)s.fontBig.h;
    s.res.fontLarge = (HFONT)s.fontLarge.h;
    s.res.fontSm    = (HFONT)s.fontSm.h;
}

inline void apply_theme(HWND hwnd, WndState& s) {
    bool dark = (s.app.theme_mode == ThemeMode::Dark) ||
                (s.app.theme_mode == ThemeMode::Auto && system_prefers_dark());
    BOOL dwm_dark = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR, &dwm_dark, sizeof(dwm_dark));
    s.active_theme = dark ? &dark_theme : &light_theme;
    s.create_brushes();
    s.tray_icon = IconObj{create_app_icon(16, dark)};
    if (s.tray_active) tray_add(hwnd, s.tray_icon);
}
