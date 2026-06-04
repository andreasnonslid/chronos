#pragma once
#include <imgui.h>
#include "config.hpp"
#include "ui_style.hpp"

struct AnalogColors {
    ImU32 hour, minute, second, background, face_fill, face_outline, tick, hour_label, center_dot;
};

AnalogColors resolve_analog_colors_imgui(const AnalogClockStyle& style, const ThemePalette& pal);

void draw_analog_clock_imgui(ImDrawList* dl, float cx, float cy, float radius,
                              const AnalogClockStyle& style, const ThemePalette& pal,
                              int hour, int minute, int second);
