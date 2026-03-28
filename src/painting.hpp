#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <vector>
#include "actions.hpp"
#include "app.hpp"
#include "formatting.hpp"
#include "layout.hpp"
#include "paint_ctx.hpp"
#include "painting_timer.hpp"
#include "theme.hpp"

// ─── Paint sub-functions ──────────────────────────────────────────────────────
inline int paint_bar(HDC hdc, int cw, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    RECT bar{0, 0, cw, layout.bar_h};
    FillRect(hdc, &bar, ctx.res.brBar);

    SelectObject(hdc, ctx.res.fontSm);
    int by = (layout.bar_h - layout.btn_h) / 2;
    int bx = (cw - (layout.w_pin + layout.w_clk + layout.w_sw + layout.w_tmr + 3 * layout.bar_gap)) / 2;
    btn(hdc, {bx, by, bx + layout.w_pin, by + layout.btn_h}, ctx.app.topmost, L"Pin", A_TOPMOST, ctx);
    bx += layout.w_pin + layout.bar_gap;
    btn(hdc, {bx, by, bx + layout.w_clk, by + layout.btn_h}, ctx.app.show_clk, L"Clock", A_SHOW_CLK, ctx);
    bx += layout.w_clk + layout.bar_gap;
    btn(hdc, {bx, by, bx + layout.w_sw, by + layout.btn_h}, ctx.app.show_sw, L"Stopwatch", A_SHOW_SW, ctx);
    bx += layout.w_sw + layout.bar_gap;
    btn(hdc, {bx, by, bx + layout.w_tmr, by + layout.btn_h}, ctx.app.show_tmr, L"Timers", A_SHOW_TMR, ctx);
    return layout.bar_h;
}

inline int paint_clock(HDC hdc, int cw, int y, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    divider(hdc, y, cw, ctx);
    SYSTEMTIME st;
    GetLocalTime(&st);
    auto buf = std::format(L"{:02}:{:02}:{:02}", st.wHour, st.wMinute, st.wSecond);
    SelectObject(hdc, ctx.res.fontBig);
    SetTextColor(hdc, ctx.theme.text);
    RECT tr{0, y, cw, y + layout.clk_h};
    DrawTextW(hdc, buf.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return y + layout.clk_h;
}

inline int paint_stopwatch(HDC hdc, int cw, int y, PaintCtx& ctx, std::chrono::steady_clock::time_point now) {
    auto& layout = ctx.layout;
    auto& th = ctx.theme;
    divider(hdc, y, cw, ctx);
    auto elap = ctx.app.sw.elapsed(now);
    std::wstring etime = format_stopwatch_display(elap);

    SelectObject(hdc, ctx.res.fontBig);
    SetTextColor(hdc, th.text);
    RECT tr{0, y + layout.dpi_scale(4), cw, y + layout.dpi_scale(44)};
    DrawTextW(hdc, etime.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, ctx.res.fontSm);
    bool running = ctx.app.sw.is_running();
    int bw = layout.dpi_scale(76), gap = layout.dpi_scale(6), bh = layout.dpi_scale(28);
    int bx0 = (cw - 3 * bw - 2 * gap) / 2;
    int by0 = y + layout.dpi_scale(46);
    btn(hdc, {bx0, by0, bx0 + bw, by0 + bh}, running, running ? L"Stop" : L"Start", A_SW_START, ctx);
    btn(hdc, {bx0 + bw + gap, by0, bx0 + 2 * bw + gap, by0 + bh}, false, L"Lap", A_SW_LAP, ctx);
    btn(hdc, {bx0 + 2 * (bw + gap), by0, bx0 + 3 * bw + 2 * gap, by0 + bh}, false, L"Reset", A_SW_RESET, ctx);

    auto& laps = ctx.app.sw.laps();
    auto info = laps.empty() ? std::wstring(L"\u2014")
                             : std::format(L"Lap {}  \u2014  {}", laps.size(), format_stopwatch_short(laps.back()));
    SetTextColor(hdc, th.dim);
    RECT ir{0, by0 + bh + layout.dpi_scale(4), cw, by0 + bh + layout.dpi_scale(22)};
    DrawTextW(hdc, info.c_str(), -1, &ir, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    bool has_file = !ctx.app.sw_lap_file.empty();
    int gbw = layout.dpi_scale(100), gbh = layout.dpi_scale(18);
    auto lap_label = ctx.app.lap_write_failed ? L"Get Laps (!)" : L"Get Laps";
    auto lap_col = ctx.app.lap_write_failed ? th.expire : has_file ? th.btn : th.dim;
    btn(hdc, {(cw - gbw) / 2, by0 + bh + layout.dpi_scale(24), (cw + gbw) / 2, by0 + bh + layout.dpi_scale(24) + gbh},
        false, lap_label, has_file ? A_SW_GET : 0, ctx, lap_col);
    return y + layout.sw_h;
}

inline void paint_help(HDC hdc, int cw, int y_bottom, PaintCtx& ctx) {
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
        {L"R", L"Reset stopwatch or first timer"},
        {L"T", L"Toggle always-on-top"},
        {L"D", L"Cycle theme: Auto \u2192 Dark \u2192 Light"},
        {L"1-3", L"Start/Stop timer 1-3"},
        {L"+ / =", L"Add a timer slot"},
        {L"-", L"Remove last timer slot"},
        {L"H / ?", L"Toggle this help"},
    };

    int line_h = layout.dpi_scale(20);
    int pad = layout.dpi_scale(12);
    int sy = layout.bar_h + pad;
    for (auto& [key, desc] : shortcuts) {
        auto line = std::format(L"  {}  \u2014  {}", key, desc);
        RECT lr{pad, sy, cw - pad, sy + line_h};
        DrawTextW(hdc, line.c_str(), -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        sy += line_h;
    }
}

// ─── Paint dispatcher ─────────────────────────────────────────────────────────
inline void paint_all(HDC hdc, int cw, int ch, PaintCtx& ctx) {
    ctx.btns.clear();
    SetBkMode(hdc, TRANSPARENT);

    RECT all{0, 0, cw, ch};
    FillRect(hdc, &all, ctx.res.brBg);

    ctx.now = std::chrono::steady_clock::now();
    auto now = ctx.now;
    int y = paint_bar(hdc, cw, ctx);
    if (ctx.app.show_clk) y = paint_clock(hdc, cw, y, ctx);
    if (ctx.app.show_sw) y = paint_stopwatch(hdc, cw, y, ctx, now);
    if (ctx.app.show_tmr) y = paint_timers(hdc, cw, y, ctx, now);
    if (ctx.app.show_help) paint_help(hdc, cw, y, ctx);
}
