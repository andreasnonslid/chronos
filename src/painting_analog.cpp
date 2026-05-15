#include "painting_analog.hpp"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include "config.hpp"
#include "gdi.hpp"
#include "layout.hpp"
#include "theme.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

COLORREF analog_pick_color(int configured, COLORREF fallback) {
    return configured >= 0 ? (COLORREF)configured : fallback;
}

COLORREF analog_blend(COLORREF fg, COLORREF bg, int opacity_pct) {
    opacity_pct = std::clamp(opacity_pct, 0, 100);
    auto mix = [&](int f, int b) { return (f * opacity_pct + b * (100 - opacity_pct)) / 100; };
    return RGB(mix(GetRValue(fg), GetRValue(bg)),
               mix(GetGValue(fg), GetGValue(bg)),
               mix(GetBValue(fg), GetBValue(bg)));
}

AnalogResolvedColors resolve_analog_colors(const AnalogClockStyle& style,
                                                   const Theme& theme) {
    COLORREF background = analog_pick_color(style.background_color, theme.bg);
    COLORREF outline = analog_pick_color(style.face_outline_color,
                         analog_pick_color(style.face_color, theme.divider));
    return {
        .hour        = analog_blend(analog_pick_color(style.hour_color, theme.text), background, style.hour_opacity_pct),
        .minute      = analog_blend(analog_pick_color(style.minute_color, theme.text), background, style.minute_opacity_pct),
        .second      = analog_blend(analog_pick_color(style.second_color, theme.dim), background, style.second_opacity_pct),
        .background  = background,
        .face_fill   = analog_blend(analog_pick_color(style.face_fill_color, background), background, style.face_opacity_pct),
        .face_outline= analog_blend(outline, background, style.face_opacity_pct),
        .tick        = analog_blend(analog_pick_color(style.tick_color, theme.dim), background, style.tick_opacity_pct),
        .hour_label  = analog_blend(analog_pick_color(style.hour_label_color,
                                          analog_pick_color(style.tick_color, theme.dim)), background, style.tick_opacity_pct),
        .center_dot  = analog_blend(analog_pick_color(style.center_dot_color,
                                          analog_pick_color(style.hour_color, theme.text)), background, style.hour_opacity_pct),
    };
}

void draw_analog_clock_native(HDC hdc, RECT area, const AnalogClockStyle& style,
                                      const Theme& theme, int dpi,
                                      int hour, int minute, int second) {
    auto colors = resolve_analog_colors(style, theme);

    int cx = (area.left + area.right) / 2;
    int cy = (area.top + area.bottom) / 2;
    int w = area.right - area.left;
    int h = area.bottom - area.top;
    int radius = (w < h ? w : h) / 2 - dpi * 6 / STANDARD_DPI;
    radius = radius * std::clamp(style.radius_pct, 50, 100) / 100;
    if (radius < 10) return;

    auto dpi_px = [&](int base) { return std::max(1, base * dpi / STANDARD_DPI); };

    auto angle_rad = [](double units, double total) {
        return (units / total) * 2.0 * M_PI - M_PI / 2.0;
    };

    auto line_from_center = [&](double angle, int len, COLORREF color, int thickness) {
        GdiObj pen{CreatePen(PS_SOLID, dpi_px(thickness), color)};
        auto* old_pen = (HPEN)SelectObject(hdc, pen.h);
        int ex = cx + (int)std::lround(std::cos(angle) * len);
        int ey = cy + (int)std::lround(std::sin(angle) * len);
        MoveToEx(hdc, cx, cy, nullptr);
        LineTo(hdc, ex, ey);
        SelectObject(hdc, old_pen);
    };

    {
        GdiObj bg_brush{CreateSolidBrush(colors.background)};
        FillRect(hdc, &area, (HBRUSH)bg_brush.h);
    }

    {
        GdiObj face_pen{CreatePen(PS_SOLID, dpi_px(1), colors.face_outline)};
        GdiObj face_brush{CreateSolidBrush(colors.face_fill)};
        auto* old_pen = (HPEN)SelectObject(hdc, face_pen.h);
        auto* old_brush = (HBRUSH)SelectObject(hdc, face_brush.h);
        Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
        SelectObject(hdc, old_pen);
        SelectObject(hdc, old_brush);
    }

    for (int i = 0; i < 60; ++i) {
        bool is_hour_mark = (i % 5 == 0);
        if (!is_hour_mark && !style.show_minute_ticks) continue;
        double a = angle_rad(i, 60.0);
        int tick_pct = is_hour_mark ? style.hour_tick_pct : style.minute_tick_pct;
        int outer = radius - dpi_px(1);
        int inner = radius - radius * tick_pct / 100;
        int thickness = is_hour_mark ? dpi_px(2) : dpi_px(1);
        GdiObj tick_pen{CreatePen(PS_SOLID, thickness, colors.tick)};
        auto* old_pen = (HPEN)SelectObject(hdc, tick_pen.h);
        int ox = cx + (int)std::lround(std::cos(a) * outer);
        int oy = cy + (int)std::lround(std::sin(a) * outer);
        int ix = cx + (int)std::lround(std::cos(a) * inner);
        int iy = cy + (int)std::lround(std::sin(a) * inner);
        MoveToEx(hdc, ox, oy, nullptr);
        LineTo(hdc, ix, iy);
        SelectObject(hdc, old_pen);
    }

    if (style.hour_labels != HourLabels::None) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, colors.hour_label);
        int label_font_h = -MulDiv(7, dpi, 72);
        GdiObj label_font{CreateFontW(label_font_h, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI")};
        auto* old_font = (HFONT)SelectObject(hdc, label_font.h);
        int label_r = radius - radius * style.hour_tick_pct / 100 - dpi_px(6);
        for (int i = 1; i <= 12; ++i) {
            if (style.hour_labels == HourLabels::Sparse && i != 12 && i != 3 && i != 6 && i != 9)
                continue;
            double a = angle_rad(i, 12.0);
            int lx = cx + (int)std::lround(std::cos(a) * label_r);
            int ly = cy + (int)std::lround(std::sin(a) * label_r);
            wchar_t buf[4];
            wsprintfW(buf, L"%d", i);
            RECT lr = {lx - dpi_px(8), ly - dpi_px(6), lx + dpi_px(8), ly + dpi_px(6)};
            DrawTextW(hdc, buf, -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        SelectObject(hdc, old_font);
    }

    double hour_angle = angle_rad(hour % 12 + minute / 60.0 + second / 3600.0, 12.0);
    if (style.hour_len_pct > 0)
        line_from_center(hour_angle, radius * style.hour_len_pct / 100, colors.hour, style.hour_thickness);

    double min_angle = angle_rad(minute + second / 60.0, 60.0);
    if (style.minute_len_pct > 0)
        line_from_center(min_angle, radius * style.minute_len_pct / 100, colors.minute, style.minute_thickness);

    double sec_angle = angle_rad(second, 60.0);
    if (style.second_len_pct > 0)
        line_from_center(sec_angle, radius * style.second_len_pct / 100, colors.second, style.second_thickness);

    if (style.center_dot_size > 0) {
        int dot_r = dpi_px(style.center_dot_size);
        GdiObj dot_brush{CreateSolidBrush(colors.center_dot)};
        GdiObj dot_pen{CreatePen(PS_NULL, 0, 0)};
        auto* old_brush = (HBRUSH)SelectObject(hdc, dot_brush.h);
        auto* old_pen = (HPEN)SelectObject(hdc, dot_pen.h);
        Ellipse(hdc, cx - dot_r, cy - dot_r, cx + dot_r, cy + dot_r);
        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
    }
}

void draw_analog_clock(HDC hdc, RECT area, const AnalogClockStyle& style,
                               const Theme& theme, int dpi,
                               int hour, int minute, int second) {
    int w = area.right - area.left;
    int h = area.bottom - area.top;
    if (w <= 0 || h <= 0) return;

    constexpr int AA_SCALE = 3;
    DcObj mem_dc{CreateCompatibleDC(hdc)};
    if (!mem_dc.h) {
        draw_analog_clock_native(hdc, area, style, theme, dpi, hour, minute, second);
        return;
    }
    GdiObj bmp{CreateCompatibleBitmap(hdc, w * AA_SCALE, h * AA_SCALE)};
    if (!bmp.h) {
        draw_analog_clock_native(hdc, area, style, theme, dpi, hour, minute, second);
        return;
    }

    auto* old_bmp = (HBITMAP)SelectObject(mem_dc.h, bmp.h);
    RECT hi = {0, 0, w * AA_SCALE, h * AA_SCALE};
    AnalogClockStyle hi_style = style;
    draw_analog_clock_native(mem_dc.h, hi, hi_style, theme, dpi * AA_SCALE, hour, minute, second);

    BOOL blit_ok = FALSE;
    int saved_dc = SaveDC(hdc);
    if (saved_dc != 0) {
        SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, 0, 0, nullptr);
        blit_ok = StretchBlt(hdc, area.left, area.top, w, h,
                             mem_dc.h, 0, 0, w * AA_SCALE, h * AA_SCALE, SRCCOPY);
        RestoreDC(hdc, saved_dc);
    } else {
        int old_mode = SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, 0, 0, nullptr);
        blit_ok = StretchBlt(hdc, area.left, area.top, w, h,
                             mem_dc.h, 0, 0, w * AA_SCALE, h * AA_SCALE, SRCCOPY);
        SetStretchBltMode(hdc, old_mode);
    }

    SelectObject(mem_dc.h, old_bmp);
    if (!blit_ok)
        draw_analog_clock_native(hdc, area, style, theme, dpi, hour, minute, second);
}
