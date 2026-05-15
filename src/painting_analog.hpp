#pragma once
#include <windows.h>
#include "config.hpp"
#include "theme.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct AnalogResolvedColors {
    COLORREF hour;
    COLORREF minute;
    COLORREF second;
    COLORREF background;
    COLORREF face_fill;
    COLORREF face_outline;
    COLORREF tick;
    COLORREF hour_label;
    COLORREF center_dot;
};

AnalogResolvedColors resolve_analog_colors(const AnalogClockStyle& style, const Theme& theme);
void draw_analog_clock(HDC hdc, RECT area, const AnalogClockStyle& style,
                       const Theme& theme, int dpi,
                       int hour, int minute, int second);
