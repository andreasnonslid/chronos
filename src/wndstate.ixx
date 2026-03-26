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
    HFONT hFontBig = nullptr;
    HFONT hFontLarge = nullptr;
    HFONT hFontSm = nullptr;
    bool fonts_custom = false;
    int timer_ms = 100;
    bool tray_active = false;
    HICON tray_icon = nullptr;

    HBRUSH brBg = nullptr;
    HBRUSH brBar = nullptr;
    HBRUSH brBtn = nullptr;
    HBRUSH brActive = nullptr;
    HBRUSH brBlink = nullptr;
    HBRUSH brFill = nullptr;
    HBRUSH brFillExp = nullptr;
    HBRUSH brHelp = nullptr;
    HPEN pnNull = nullptr;
    HPEN pnDivider = nullptr;

    void create_brushes() {
        auto& th = *active_theme;
        brBg = CreateSolidBrush(th.bg);
        brBar = CreateSolidBrush(th.bar);
        brBtn = CreateSolidBrush(th.btn);
        brActive = CreateSolidBrush(th.active);
        brBlink = CreateSolidBrush(th.blink);
        brFill = CreateSolidBrush(th.fill);
        brFillExp = CreateSolidBrush(th.fill_exp);
        brHelp = CreateSolidBrush(th.help_bg);
        pnNull = CreatePen(PS_NULL, 0, 0);
        pnDivider = CreatePen(PS_SOLID, 1, th.divider);
    }

    void destroy_brushes() {
        HGDIOBJ objs[] = {brBg, brBar, brBtn, brActive, brBlink, brFill, brFillExp, brHelp, pnNull, pnDivider};
        for (auto h : objs)
            if (h) DeleteObject(h);
    }

    ~WndState() {
        if (fonts_custom) {
            DeleteObject(hFontBig);
            DeleteObject(hFontLarge);
            DeleteObject(hFontSm);
        }
        destroy_brushes();
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
            .brBg = brBg,
            .brBar = brBar,
            .brBtn = brBtn,
            .brActive = brActive,
            .brBlink = brBlink,
            .brFill = brFill,
            .brFillExp = brFillExp,
            .brHelp = brHelp,
            .pnNull = pnNull,
            .pnDivider = pnDivider,
        };
    }
};

export void recreate_fonts(WndState& s) {
    HFONT newBig = make_font(26, true, s.layout);
    HFONT newLarge = make_font(34, true, s.layout);
    HFONT newSm = make_font(11, false, s.layout);
    if (!newBig || !newLarge || !newSm) {
        if (newBig) DeleteObject(newBig);
        if (newLarge) DeleteObject(newLarge);
        if (newSm) DeleteObject(newSm);
        if (!s.fonts_custom) {
            s.hFontBig = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            s.hFontLarge = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            s.hFontSm = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        return;
    }
    if (s.fonts_custom) {
        DeleteObject(s.hFontBig);
        DeleteObject(s.hFontLarge);
        DeleteObject(s.hFontSm);
    }
    s.hFontBig = newBig;
    s.hFontLarge = newLarge;
    s.hFontSm = newSm;
    s.fonts_custom = true;
}
