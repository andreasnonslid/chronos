#include "painting_scene.hpp"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cwchar>
#include <string>
#include "actions.hpp"
#include "app.hpp"
#include "encoding.hpp"
#include "layout.hpp"
#include "painting_analog.hpp"
#include "theme.hpp"
#include "ui_scene.hpp"
#include "ui_windows_painter.hpp"

namespace {

RECT to_rect(const ui_scene::RectI& r) {
    return RECT{r.left, r.top, r.right, r.bottom};
}

HFONT font_for(ui_scene::TextStyle style, const PaintCtx& ctx) {
    switch (style) {
    case ui_scene::TextStyle::Big:   return ctx.res.fontBig;
    case ui_scene::TextStyle::Large: return ctx.res.fontLarge;
    case ui_scene::TextStyle::Small: return ctx.res.fontSm;
    }
    return ctx.res.fontSm;
}

// Picks the largest font height (in pixels) whose rendering of `text` fits
// within `rc`. Returns a newly-created HFONT that the caller must DeleteObject.
HFONT make_autofit_font(HDC hdc, const wchar_t* text, const RECT& rc) {
    int rect_w = rc.right - rc.left;
    int rect_h = rc.bottom - rc.top;
    int max_h = std::max(8, rect_h * 8 / 10);
    int min_h = 8;
    int max_w = std::max(8, rect_w - 8);
    int h = max_h;
    while (h > min_h) {
        HFONT f = CreateFontW(-h, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, f);
        SIZE sz{};
        GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &sz);
        SelectObject(hdc, old);
        if (sz.cx <= max_w) return f;
        DeleteObject(f);
        int next = h * 9 / 10;
        if (next >= h) --next;
        h = next;
    }
    return CreateFontW(-min_h, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

UINT text_format(ui_scene::Align align) {
    switch (align) {
    case ui_scene::Align::Left:   return DT_LEFT   | DT_VCENTER | DT_SINGLELINE;
    case ui_scene::Align::Center: return DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    case ui_scene::Align::Right:  return DT_RIGHT  | DT_VCENTER | DT_SINGLELINE;
    }
    return DT_LEFT | DT_VCENTER | DT_SINGLELINE;
}

void render_op(HDC hdc, const ui_scene::Op& op, PaintCtx& ctx) {
    using ui_scene::OpKind;
    RECT rc = to_rect(op.rect);
    switch (op.kind) {
    case OpKind::FillRect:
        win_fill_rect(hdc, rc, op.fill);
        break;
    case OpKind::Divider:
        win_paint_divider(hdc, op.rect.left, op.rect.right, op.rect.top, DividerPaint{.color = op.stroke});
        break;
    case OpKind::Progress:
        win_paint_progress(hdc, rc, ProgressPaint{.fill = op.fill});
        break;
    case OpKind::Text: {
        std::wstring w = utf8_to_wide(op.text);
        UINT fmt = text_format(op.align);
        if (op.end_ellipsis) fmt |= DT_END_ELLIPSIS;
        HFONT fit = op.auto_fit ? make_autofit_font(hdc, w.c_str(), rc) : nullptr;
        win_paint_text(hdc, rc, w.c_str(), fit ? fit : font_for(op.text_style, ctx),
                       TextPaint{.color = op.text_color}, fmt);
        if (fit) DeleteObject(fit);
        break;
    }
    case OpKind::Button: {
        std::wstring w = utf8_to_wide(op.text);
        WidgetPaint paint{
            .fill = op.fill,
            .text = op.text_color,
            .border = op.stroke,
            .radius_px = op.radius_px,
        };
        win_paint_button(hdc, rc, w.c_str(), font_for(op.text_style, ctx), paint, text_format(op.align));
        break;
    }
    }
    if (op.id != 0) ctx.btns.push_back({rc, op.id});
}

} // namespace

void paint_scene(HDC hdc, const ui_scene::Scene& scene, PaintCtx& ctx) {
    for (const auto& op : scene.ops) render_op(hdc, op, ctx);
    if (scene.analog_clock) {
        const auto& ac = *scene.analog_clock;
        RECT rc = to_rect(ac.rect);
        draw_analog_clock(hdc, rc, ac.style, ctx.theme, ctx.layout.dpi, ac.hour, ac.minute, ac.second);
        if (ac.id != 0) ctx.btns.push_back({rc, ac.id});
    }
}

void paint_all(HDC hdc, int cw, int ch, PaintCtx& ctx) {
    ctx.btns.clear();
    SetBkMode(hdc, TRANSPARENT);

    RECT all{0, 0, cw, ch};
    FillRect(hdc, &all, ctx.res.brBg);

    ctx.now = std::chrono::steady_clock::now();

    SYSTEMTIME st;
    GetLocalTime(&st);
    UiMakers ui = make_ui(ctx.theme.palette);
    auto scene_state = ui_scene::main_scene_state_from_app(ctx.app, ctx.now, st.wHour, st.wMinute, st.wSecond,
                                                           ctx.global_hotkey_ok);
    auto scene = ui_scene::build_main_scene(ctx.layout, cw, scene_state, ui);
    paint_scene(hdc, scene, ctx);
}
