module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <filesystem>
#include <utility>
#include <vector>
export module wndstate;
import app;
import gdi;
import layout;
import painting;
import theme;

static HFONT make_font(int pt, bool bold, const Layout& layout) {
    int h = -MulDiv(pt, layout.dpi, 72);
    return CreateFontW(h, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

export struct WndState {
    App app;
    Layout layout;
    std::filesystem::path cfg_path;
    const Theme* active_theme = &dark_theme;
    std::vector<std::pair<RECT, int>> btns;
    GdiObj fontBig{nullptr}, fontLarge{nullptr}, fontSm{nullptr};
    HFONT hFontBig = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hFontLarge = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hFontSm = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    int timer_ms = 100;
    bool tray_active = false;
    HICON tray_icon = nullptr;

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
    }

    ~WndState() {
        if (mdc) DeleteDC(mdc);
        if (buf_bmp) DeleteObject(buf_bmp);
    }

    WndState(const WndState&) = delete;
    WndState& operator=(const WndState&) = delete;
    WndState() = default;

    HDC mdc = nullptr;
    HBITMAP buf_bmp = nullptr;
    int buf_w = 0, buf_h = 0;

    void ensure_buffer(HDC hdc, int w, int h) {
        if (w != buf_w || h != buf_h) {
            HDC new_mdc = CreateCompatibleDC(hdc);
            if (!new_mdc) return;
            HBITMAP new_bmp = CreateCompatibleBitmap(hdc, w, h);
            if (!new_bmp) {
                DeleteDC(new_mdc);
                return;
            }
            if (mdc) DeleteDC(mdc);
            if (buf_bmp) DeleteObject(buf_bmp);
            mdc = new_mdc;
            buf_bmp = new_bmp;
            SelectObject(mdc, buf_bmp);
            buf_w = w;
            buf_h = h;
        }
    }

    PaintCtx paint_ctx() {
        return {
            .app = app,
            .layout = layout,
            .theme = *active_theme,
            .btns = btns,
            .fontBig = hFontBig,
            .fontLarge = hFontLarge,
            .fontSm = hFontSm,
            .brBg      = (HBRUSH)brBg.h,
            .brBar     = (HBRUSH)brBar.h,
            .brBtn     = (HBRUSH)brBtn.h,
            .brActive  = (HBRUSH)brActive.h,
            .brBlink   = (HBRUSH)brBlink.h,
            .brFill    = (HBRUSH)brFill.h,
            .brFillExp = (HBRUSH)brFillExp.h,
            .brHelp    = (HBRUSH)brHelp.h,
            .pnNull    = (HPEN)pnNull.h,
            .pnDivider = (HPEN)pnDivider.h,
        };
    }
};

static constexpr int FONT_PT_BIG = 26;
static constexpr int FONT_PT_LARGE = 34;
static constexpr int FONT_PT_SM = 11;

export void recreate_fonts(WndState& s) {
    GdiObj newBig{make_font(FONT_PT_BIG, true, s.layout)};
    GdiObj newLarge{make_font(FONT_PT_LARGE, true, s.layout)};
    GdiObj newSm{make_font(FONT_PT_SM, false, s.layout)};
    if (!newBig.h || !newLarge.h || !newSm.h) return;
    s.fontBig = std::move(newBig);
    s.fontLarge = std::move(newLarge);
    s.fontSm = std::move(newSm);
    s.hFontBig = (HFONT)s.fontBig.h;
    s.hFontLarge = (HFONT)s.fontLarge.h;
    s.hFontSm = (HFONT)s.fontSm.h;
}
