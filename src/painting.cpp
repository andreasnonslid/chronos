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

int paint_alarms(HDC hdc, int cw, int y, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    auto& th = ctx.theme;
    divider(hdc, y, cw, ctx);

    int pad = layout.dpi_scale(10);
    int bw  = layout.dpi_scale(56);
    int bh  = layout.dpi_scale(24);
    int by0 = y + (layout.alarm_header_h - bh) / 2;

    SelectObject(hdc, ctx.res.fontSm);
    SetTextColor(hdc, th.text);
    RECT title_r{pad, y, cw - bw - 2 * pad, y + layout.alarm_header_h};
    DrawTextW(hdc, L"Alarms", -1, &title_r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    btn(hdc, {cw - bw - pad, by0, cw - pad, by0 + bh}, false, L"+ Add", A_ALARM_ADD, ctx);

    int row_y = y + layout.alarm_header_h;
    int del_w = layout.dpi_scale(36);
    int tog_w = layout.dpi_scale(50);

    HBRUSH stripe = CreateSolidBrush(th.bar);
    for (int i = 0; i < (int)ctx.app.alarms.size(); ++i) {
        const auto& a = ctx.app.alarms[i];

        RECT row_bg{0, row_y, cw, row_y + layout.alarm_row_h};
        if (i % 2 == 1)
            FillRect(hdc, &row_bg, stripe);

        std::wstring sched;
        if (a.schedule == AlarmSchedule::Days) {
            const wchar_t* day_names[] = {L"Mo", L"Tu", L"We", L"Th", L"Fr", L"Sa", L"Su"};
            for (int d = 0; d < 7; ++d)
                if (a.days_mask & (1 << d)) {
                    if (!sched.empty()) sched += L' ';
                    sched += day_names[d];
                }
            if (sched.empty()) sched = L"(no days)";
        } else {
            sched = std::format(L"{:04}-{:02}-{:02}", a.date_year, a.date_month, a.date_day);
        }
        std::wstring label;
        {
            std::wstring name_w = utf8_to_wide(a.name);
            if (!name_w.empty()) label = name_w + L"  ";
        }
        label += std::format(L"{:02}:{:02}  {}", a.hour, a.minute, sched);

        int text_x = pad;
        int row_mid = row_y + layout.alarm_row_h / 2;
        int text_h  = layout.dpi_scale(18);
        int buttons_right = cw - pad;
        int del_x  = buttons_right - del_w;
        int tog_x  = del_x - layout.dpi_scale(6) - tog_w;

        RECT lr{text_x, row_mid - text_h / 2, tog_x - layout.dpi_scale(6), row_mid + text_h / 2};
        SetTextColor(hdc, a.enabled ? th.text : th.dim);
        DrawTextW(hdc, label.c_str(), -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        int btn_top = row_mid - bh / 2;
        btn(hdc, {tog_x, btn_top, tog_x + tog_w, btn_top + bh},
            a.enabled, a.enabled ? L"On" : L"Off", A_ALARM_TOGGLE + i, ctx);
        btn(hdc, {del_x, btn_top, del_x + del_w, btn_top + bh},
            false, L"Del", A_ALARM_DEL + i, ctx);

        row_y += layout.alarm_row_h;
    }
    DeleteObject(stripe);

    if (ctx.app.alarms.empty()) {
        SetTextColor(hdc, th.dim);
        RECT er{pad, row_y, cw - pad, row_y + layout.alarm_row_h};
        DrawTextW(hdc, L"No alarms set", -1, &er, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        row_y += layout.alarm_row_h;
    }

    return row_y;
}

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
    paint_scene(hdc, scene, ctx);

    if (analog_clock) paint_analog_panel(hdc, cw, analog_y, ctx);
    if (ctx.app.show_alarms) y = paint_alarms(hdc, cw, y, ctx);
    if (ctx.app.show_help) paint_help(hdc, cw, y, ctx);
}
