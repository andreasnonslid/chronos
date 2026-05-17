#pragma once
#include <windows.h>

#include "gdi.hpp"
#include "theme.hpp"
#include "ui_style.hpp"

inline GdiObj win_brush(UiColor color) { return GdiObj{CreateSolidBrush(colorref_from_ui(color))}; }

inline GdiObj win_pen(UiColor color, int width = 1, int style = PS_SOLID) {
    return GdiObj{CreatePen(style, width, colorref_from_ui(color))};
}

inline void win_fill_rect(HDC hdc, const RECT& rc, UiColor color) {
    GdiObj brush = win_brush(color);
    FillRect(hdc, &rc, (HBRUSH)brush.h);
}

inline void win_paint_text(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, const TextPaint& paint,
                           UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, colorref_from_ui(paint.color));
    if (font) SelectObject(hdc, font);
    RECT text_rc = rc;
    DrawTextW(hdc, text, -1, &text_rc, format);
}

inline void win_paint_label(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, const ThemePalette& palette,
                            UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    win_paint_text(hdc, rc, text, font, make_ui(palette).text(), format);
}

inline void win_paint_divider(HDC hdc, int x0, int x1, int y, const DividerPaint& paint) {
    GdiObj pen = win_pen(paint.color);
    auto* old_pen = (HPEN)SelectObject(hdc, pen.h);
    MoveToEx(hdc, x0, y, nullptr);
    LineTo(hdc, x1, y);
    SelectObject(hdc, old_pen);
}

inline void win_paint_progress(HDC hdc, const RECT& rc, const ProgressPaint& paint) {
    win_fill_rect(hdc, rc, paint.fill);
}

inline void win_paint_button(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, const WidgetPaint& paint,
                             UINT text_format = DT_CENTER | DT_VCENTER | DT_SINGLELINE) {
    GdiObj brush = win_brush(paint.fill);
    GdiObj pen = win_pen(paint.border);
    auto* old_brush = (HBRUSH)SelectObject(hdc, brush.h);
    auto* old_pen = (HPEN)SelectObject(hdc, pen.h);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, paint.radius_px, paint.radius_px);
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);

    win_paint_text(hdc, rc, text, font, TextPaint{.color = paint.text}, text_format);
}
