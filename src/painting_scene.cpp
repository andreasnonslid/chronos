#include "painting_scene.hpp"
#include <windows.h>
#include <string>
#include "encoding.hpp"
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
        win_paint_text(hdc, rc, w.c_str(), font_for(op.text_style, ctx), TextPaint{.color = op.text_color}, fmt);
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
}
