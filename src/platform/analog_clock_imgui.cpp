#include "analog_clock_imgui.hpp"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "config.hpp"
#include "ui_style.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static ImU32 to_im(UiColor c, int opacity_pct = 100) {
    int a = std::clamp(opacity_pct * 255 / 100, 0, 255);
    return IM_COL32(c.r, c.g, c.b, a);
}

static UiColor pick(int configured, UiColor fallback) {
    if (configured >= 0) {
        return UiColor{(uint8_t)((configured >> 0) & 0xFF),
                       (uint8_t)((configured >> 8) & 0xFF),
                       (uint8_t)((configured >> 16) & 0xFF)};
    }
    return fallback;
}

static UiColor blend(UiColor fg, UiColor bg, int pct) {
    pct = std::clamp(pct, 0, 100);
    auto mix = [&](int f, int b) -> uint8_t { return (uint8_t)((f * pct + b * (100 - pct)) / 100); };
    return {mix(fg.r, bg.r), mix(fg.g, bg.g), mix(fg.b, bg.b)};
}

AnalogColors resolve_analog_colors_imgui(const AnalogClockStyle& style, const ThemePalette& pal) {
    UiColor bg_col = pick(style.background_color, pal.bg);
    UiColor outline_col = pick(style.face_outline_color, pick(style.face_color, pal.divider));
    UiColor text_col = pal.text;
    UiColor tick_col = pal.dim;

    return {
        .hour        = to_im(blend(pick(style.hour_color,   text_col), bg_col, style.hour_opacity_pct)),
        .minute      = to_im(blend(pick(style.minute_color, text_col), bg_col, style.minute_opacity_pct)),
        .second      = to_im(blend(pick(style.second_color, tick_col), bg_col, style.second_opacity_pct)),
        .background  = to_im(bg_col),
        .face_fill   = to_im(blend(pick(style.face_fill_color, bg_col), bg_col, style.face_opacity_pct)),
        .face_outline= to_im(blend(outline_col, bg_col, style.face_opacity_pct)),
        .tick        = to_im(blend(pick(style.tick_color, tick_col), bg_col, style.tick_opacity_pct)),
        .hour_label  = to_im(blend(pick(style.hour_label_color,
                               pick(style.tick_color, tick_col)), bg_col, style.tick_opacity_pct)),
        .center_dot  = to_im(blend(pick(style.center_dot_color,
                               pick(style.hour_color, text_col)), bg_col, style.hour_opacity_pct)),
    };
}

void draw_analog_clock_imgui(ImDrawList* dl, float cx, float cy, float radius,
                              const AnalogClockStyle& style, const ThemePalette& pal,
                              int hour, int minute, int second) {
    if (radius < 10.f) return;
    auto colors = resolve_analog_colors_imgui(style, pal);

    auto angle = [](double units, double total) -> float {
        return (float)((units / total) * 2.0 * M_PI - M_PI / 2.0);
    };
    auto px = [&](float base, float r) -> float { return cx + std::cos(r) * base; };
    auto py = [&](float base, float r) -> float { return cy + std::sin(r) * base; };

    // Background
    dl->AddRectFilled({cx - radius - 2, cy - radius - 2},
                      {cx + radius + 2, cy + radius + 2}, colors.background);
    // Face fill + outline
    dl->AddCircleFilled({cx, cy}, radius, colors.face_fill, 64);
    dl->AddCircle({cx, cy}, radius, colors.face_outline, 64, 1.5f);

    // Tick marks
    for (int i = 0; i < 60; ++i) {
        bool is_hour = (i % 5 == 0);
        if (!is_hour && !style.show_minute_ticks) continue;
        float a = angle(i, 60.0);
        int tp = is_hour ? style.hour_tick_pct : style.minute_tick_pct;
        float outer = radius - 1.f;
        float inner = radius - radius * tp / 100.f;
        float thick = is_hour ? 2.f : 1.f;
        dl->AddLine({px(outer, a), py(outer, a)}, {px(inner, a), py(inner, a)},
                    colors.tick, thick);
    }

    // Hour labels
    if (style.hour_labels != HourLabels::None) {
        float label_r = radius - radius * style.hour_tick_pct / 100.f - 10.f;
        for (int i = 1; i <= 12; ++i) {
            if (style.hour_labels == HourLabels::Sparse && i != 12 && i != 3 && i != 6 && i != 9)
                continue;
            float a = angle(i, 12.0);
            float lx = cx + std::cos(a) * label_r;
            float ly = cy + std::sin(a) * label_r;
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", i);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            dl->AddText({lx - ts.x / 2.f, ly - ts.y / 2.f}, colors.hour_label, buf);
        }
    }

    // Hands
    auto draw_hand = [&](double units, double total, float len_pct, ImU32 col, float thick) {
        if (len_pct <= 0) return;
        float a = angle(units, total);
        float len = radius * len_pct / 100.f;
        dl->AddLine({cx, cy}, {px(len, a), py(len, a)}, col, thick);
    };

    draw_hand(hour % 12 + minute / 60.0 + second / 3600.0, 12.0,
              (float)style.hour_len_pct, colors.hour, (float)style.hour_thickness);
    draw_hand(minute + second / 60.0, 60.0,
              (float)style.minute_len_pct, colors.minute, (float)style.minute_thickness);
    draw_hand(second, 60.0,
              (float)style.second_len_pct, colors.second, (float)style.second_thickness);

    // Center dot
    if (style.center_dot_size > 0)
        dl->AddCircleFilled({cx, cy}, (float)style.center_dot_size, colors.center_dot, 16);
}
