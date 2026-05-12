#pragma once
#include <windows.h>
#include <cmath>
#include "config.hpp"
#include "gdi.hpp"
#include "layout.hpp"
#include "theme.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct AnalogResolvedColors {
    COLORREF hour;
    COLORREF minute;
    COLORREF second;
    COLORREF face;
    COLORREF tick;
};

inline AnalogResolvedColors resolve_analog_colors(const AnalogClockStyle& style,
                                                   const Theme& theme) {
    return {
        .hour   = style.hour_color >= 0 ? (COLORREF)style.hour_color : theme.text,
        .minute = style.minute_color >= 0 ? (COLORREF)style.minute_color : theme.text,
        .second = style.second_color >= 0 ? (COLORREF)style.second_color : theme.dim,
        .face   = style.face_color >= 0 ? (COLORREF)style.face_color : theme.divider,
        .tick   = style.tick_color >= 0 ? (COLORREF)style.tick_color : theme.dim,
    };
}

inline void draw_analog_clock(HDC hdc, RECT area, const AnalogClockStyle& style,
                               const Theme& theme, int dpi,
                               int hour, int minute, int second) {
    auto colors = resolve_analog_colors(style, theme);

    int cx = (area.left + area.right) / 2;
    int cy = (area.top + area.bottom) / 2;
    int w = area.right - area.left;
    int h = area.bottom - area.top;
    int radius = (w < h ? w : h) / 2 - dpi * 6 / STANDARD_DPI;
    if (radius < 10) return;

    auto dpi_px = [&](int base) { return base * dpi / STANDARD_DPI; };

    auto angle_rad = [](double units, double total) {
        return (units / total) * 2.0 * M_PI - M_PI / 2.0;
    };

    auto line_from_center = [&](double angle, int len, COLORREF color, int thickness) {
        GdiObj pen{CreatePen(PS_SOLID, dpi_px(thickness), color)};
        auto* old_pen = (HPEN)SelectObject(hdc, pen.h);
        int ex = cx + (int)(std::cos(angle) * len);
        int ey = cy + (int)(std::sin(angle) * len);
        MoveToEx(hdc, cx, cy, nullptr);
        LineTo(hdc, ex, ey);
        SelectObject(hdc, old_pen);
    };

    {
        GdiObj face_pen{CreatePen(PS_SOLID, dpi_px(1), colors.face)};
        HBRUSH null_brush = (HBRUSH)GetStockObject(NULL_BRUSH);
        auto* old_pen = (HPEN)SelectObject(hdc, face_pen.h);
        auto* old_brush = (HBRUSH)SelectObject(hdc, null_brush);
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
        int ox = cx + (int)(std::cos(a) * outer);
        int oy = cy + (int)(std::sin(a) * outer);
        int ix = cx + (int)(std::cos(a) * inner);
        int iy = cy + (int)(std::sin(a) * inner);
        MoveToEx(hdc, ox, oy, nullptr);
        LineTo(hdc, ix, iy);
        SelectObject(hdc, old_pen);
    }

    if (style.hour_labels != HourLabels::None) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, colors.tick);
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
            int lx = cx + (int)(std::cos(a) * label_r);
            int ly = cy + (int)(std::sin(a) * label_r);
            wchar_t buf[4];
            wsprintfW(buf, L"%d", i);
            RECT lr = {lx - dpi_px(8), ly - dpi_px(6), lx + dpi_px(8), ly + dpi_px(6)};
            DrawTextW(hdc, buf, -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        SelectObject(hdc, old_font);
    }

    double hour_angle = angle_rad(hour % 12 + minute / 60.0 + second / 3600.0, 12.0);
    int hour_len = radius * style.hour_len_pct / 100;
    line_from_center(hour_angle, hour_len, colors.hour, style.hour_thickness);

    double min_angle = angle_rad(minute + second / 60.0, 60.0);
    int min_len = radius * style.minute_len_pct / 100;
    line_from_center(min_angle, min_len, colors.minute, style.minute_thickness);

    double sec_angle = angle_rad(second, 60.0);
    int sec_len = radius * style.second_len_pct / 100;
    line_from_center(sec_angle, sec_len, colors.second, style.second_thickness);

    {
        int dot_r = dpi_px(3);
        GdiObj dot_brush{CreateSolidBrush(colors.hour)};
        GdiObj dot_pen{CreatePen(PS_NULL, 0, 0)};
        auto* old_brush = (HBRUSH)SelectObject(hdc, dot_brush.h);
        auto* old_pen = (HPEN)SelectObject(hdc, dot_pen.h);
        Ellipse(hdc, cx - dot_r, cy - dot_r, cx + dot_r, cy + dot_r);
        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
    }
}
