#include "painting.hpp"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <vector>
#include "assert.hpp"
#include "actions.hpp"
#include "encoding.hpp"
#include "app.hpp"
#include "formatting.hpp"
#include "layout.hpp"
#include "paint_ctx.hpp"
#include "painting_analog.hpp"
#include "painting_scene.hpp"
#include "theme.hpp"
#include "ui_scene.hpp"

namespace {

// Wall-clock string for the digital views. The Scene renderer paints this; the
// analog face is drawn by paint_analog_panel because it's not yet a Scene Op.
std::string current_clock_text(ClockView view) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    return format_clock_text(view, st.wHour, st.wMinute, st.wSecond);
}

int paint_analog_panel(HDC hdc, int cw, int y, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    divider(hdc, y, cw, ctx);
    SYSTEMTIME st;
    GetLocalTime(&st);
    RECT area{0, y, cw, y + layout.analog_clk_h};
    draw_analog_clock(hdc, area, ctx.app.analog_style, ctx.theme,
                      layout.dpi, st.wHour, st.wMinute, st.wSecond);
    ctx.btns.push_back({area, A_CLK_CYCLE});
    return y + layout.analog_clk_h;
}

} // namespace

void paint_help(HDC hdc, int cw, int y_bottom, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    RECT cr{0, layout.bar_h, cw, y_bottom > layout.bar_h ? y_bottom : layout.bar_h + layout.dpi_scale(200)};
    FillRect(hdc, &cr, ctx.res.brHelp);

    SelectObject(hdc, ctx.res.fontSm);
    SetTextColor(hdc, ctx.theme.text);

    struct {
        const wchar_t* key;
        const wchar_t* desc;
    } shortcuts[] = {
        {L"Space", L"Start/Stop stopwatch or first timer"},
        {L"L", L"Record lap"},
        {L"E", L"Open exported laps file"},
        {L"C", L"Copy laps to clipboard"},
        {L"R", L"Reset stopwatch or first timer"},
        {L"P", L"Toggle Pomodoro on first timer"},
        {L"N", L"Skip to next Pomodoro phase"},
        {L"T", L"Toggle always-on-top"},
        {L"D", L"Cycle theme: Auto → Dark → Light"},
        {L"1-9", L"Start/Stop timer 1-9"},
        {L"Shift+1-9", L"Reset timer 1-9"},
        {L"Shift+R", L"Reset all timers"},
        {L"+ / =", L"Add a timer slot"},
        {L"-", L"Remove last timer slot"},
        {L"H / ?", L"Toggle this help"},
        {L"Ctrl+Shift+Space", ctx.global_hotkey_ok ? L"Global start/stop (any app)"
                                                    : L"Global start/stop (unavailable)"},
        {L"", L""},
        {L"Scroll", L"Adjust timer value (H/M/S)"},
        {L"Click clock", L"Cycle clock format"},
        {L"Dbl-click", L"Edit timer label"},
        {L"Right-click", L"Timer presets / Pomodoro (untouched)"},
    };

    int line_h = layout.dpi_scale(20);
    int pad = layout.dpi_scale(12);
    int sy = layout.bar_h + pad;
    for (auto& [key, desc] : shortcuts) {
        auto line = std::format(L"  {}  —  {}", key, desc);
        RECT lr{pad, sy, cw - pad, sy + line_h};
        DrawTextW(hdc, line.c_str(), -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        sy += line_h;
    }
}

void paint_all(HDC hdc, int cw, int ch, PaintCtx& ctx) {
    ctx.btns.clear();
    SetBkMode(hdc, TRANSPARENT);

    RECT all{0, 0, cw, ch};
    FillRect(hdc, &all, ctx.res.brBg);

    ctx.now = std::chrono::steady_clock::now();
    auto now = ctx.now;

    // Build a partial Scene for the parts that are already modelled: the
    // toolbar, and the digital clock readout. Everything else is still painted
    // by the per-module functions below; subsequent commits migrate them.
    UiMakers ui = make_ui(ctx.theme.palette);
    auto scene_state = ui_scene::main_scene_state_from_app(ctx.app, now, current_clock_text(ctx.app.clock_view));
    ui_scene::Scene scene;
    add_toolbar(scene, ctx.layout, cw, scene_state, ui);
    int y = ctx.layout.bar_h;
    bool digital_clock = ctx.app.show_clk && ctx.app.clock_view != ClockView::Analog;
    if (digital_clock) add_clock(scene, ctx.layout, cw, y, scene_state, ui);
    // Analog clock isn't a Scene Op yet; reserve its strip in the layout flow
    // and draw the face below after paint_scene.
    bool analog_clock = ctx.app.show_clk && ctx.app.clock_view == ClockView::Analog;
    int analog_y = y;
    if (analog_clock) y += ctx.layout.analog_clk_h;
    add_stopwatch(scene, ctx.layout, cw, y, scene_state, ui);
    if (ctx.app.show_tmr) {
        for (int i = 0; i < (int)scene_state.timers.size(); ++i)
            add_timer(scene, ctx.layout, cw, y, i, scene_state.timers[i], ui, scene_state.blink_act);
    }
    add_alarms(scene, ctx.layout, cw, y, scene_state, ui);
    paint_scene(hdc, scene, ctx);

    if (analog_clock) paint_analog_panel(hdc, cw, analog_y, ctx);
    if (ctx.app.show_help) paint_help(hdc, cw, y, ctx);
}
