module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <chrono>
#include <format>
#include <optional>
#include <string>
#include <vector>
export module painting_timer;
import actions;
import app;
import config;
import formatting;
import gdi;
import layout;
import theme;
import timer;

using namespace std::chrono;
using sc = steady_clock;

// ─── Paint context ─────────────────────────────────────────────────
export struct PaintCtx {
    App& app;
    Layout& layout;
    const Theme& theme;
    std::vector<std::pair<RECT, int>>& btns;
    sc::time_point now;
    HFONT fontBig;
    HFONT fontLarge;
    HFONT fontSm;
    HBRUSH brBg;
    HBRUSH brBar;
    HBRUSH brBtn;
    HBRUSH brActive;
    HBRUSH brBlink;
    HBRUSH brFill;
    HBRUSH brFillExp;
    HBRUSH brHelp;
    HPEN pnNull;
    HPEN pnDivider;
};

// ─── Helpers ─────────────────────────────────────────────────────
export RECT btn(HDC hdc, RECT r, bool active, const wchar_t* label, int id, PaintCtx& ctx,
                std::optional<COLORREF> override_col = std::nullopt) {
    auto& layout = ctx.layout;
    bool blinking = id && ctx.app.blink_act == id && (ctx.now - ctx.app.blink_t) < BLINK_DUR;
    HBRUSH brush;
    GdiObj dyn_br{nullptr};
    if (blinking) {
        brush = ctx.brBlink;
    } else if (override_col.has_value()) {
        dyn_br.h = CreateSolidBrush(*override_col);
        brush = (HBRUSH)dyn_br.h;
    } else {
        brush = active ? ctx.brActive : ctx.brBtn;
    }
    auto* obr = (HBRUSH)SelectObject(hdc, brush);
    auto* opn = (HPEN)SelectObject(hdc, ctx.pnNull);
    int rr = layout.dpi_scale(6);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rr, rr);
    SelectObject(hdc, obr);
    SelectObject(hdc, opn);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ctx.theme.text);
    DrawTextW(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (id) ctx.btns.push_back({r, id});
    return r;
}

export void divider(HDC hdc, int y, int cw, const PaintCtx& ctx) {
    auto* op = (HPEN)SelectObject(hdc, ctx.pnDivider);
    MoveToEx(hdc, 0, y, nullptr);
    LineTo(hdc, cw, y);
    SelectObject(hdc, op);
}

// ─── Timer painters ───────────────────────────────────────────────────────
static void paint_timer_progress(HDC hdc, int cw, int y, int i, sc::time_point now, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    auto& ts = ctx.app.timers[i];
    bool expired = ts.t.expired(now);
    HBRUSH fillbr = expired ? ctx.brFillExp : ctx.brFill;
    int fw = cw;
    if (!expired) {
        auto total = duration_cast<microseconds>(ts.dur).count();
        auto rem = duration_cast<microseconds>(ts.t.remaining(now)).count();
        fw = total > 0 ? (int)(cw * (double)(total - rem) / total) : 0;
    }
    RECT fr{0, y, fw, y + layout.tmr_h};
    FillRect(hdc, &fr, fillbr);
}

static void paint_timer_idle(HDC hdc, int cw, int y, int i, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    auto& th = ctx.theme;
    auto& ts = ctx.app.timers[i];
    int abw = layout.dpi_scale(34), abh = layout.dpi_scale(16);
    int up_off = layout.dpi_scale(4);
    int td_off = up_off + abh + layout.dpi_scale(2);
    int dn_off = td_off + layout.dpi_scale(40) + layout.dpi_scale(2);
    int col_gap = layout.dpi_scale(52);
    int hh_cx = cw / 2 - col_gap, mm_cx = cw / 2, ss_cx = cw / 2 + col_gap;

    SelectObject(hdc, ctx.fontSm);
    btn(hdc, {hh_cx - abw / 2, y + up_off, hh_cx + abw / 2, y + up_off + abh}, false, L"\u25B2",
        tmr_act(i, A_TMR_HUP), ctx);
    btn(hdc, {mm_cx - abw / 2, y + up_off, mm_cx + abw / 2, y + up_off + abh}, false, L"\u25B2",
        tmr_act(i, A_TMR_MUP), ctx);
    btn(hdc, {ss_cx - abw / 2, y + up_off, ss_cx + abw / 2, y + up_off + abh}, false, L"\u25B2",
        tmr_act(i, A_TMR_SUP), ctx);

    if (!ts.label.empty()) {
        SetTextColor(hdc, th.dim);
        RECT lr{0, y + layout.dpi_scale(2), cw, y + up_off + abh};
        DrawTextW(hdc, ts.label.c_str(), -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, ctx.fontBig);
    SetTextColor(hdc, th.text);
    int field_h = layout.dpi_scale(40);
    int field_half = layout.dpi_scale(22);
    long long total_s_edit = ts.dur.count();
    auto h_s = std::format(L"{}", total_s_edit / 3600);
    auto mm_s = std::format(L"{:02}", (total_s_edit / 60) % 60);
    auto ss_s = std::format(L"{:02}", total_s_edit % 60);
    RECT hr{hh_cx - field_half, y + td_off, hh_cx + field_half, y + td_off + field_h};
    RECT mr{mm_cx - field_half, y + td_off, mm_cx + field_half, y + td_off + field_h};
    RECT sr{ss_cx - field_half, y + td_off, ss_cx + field_half, y + td_off + field_h};
    DrawTextW(hdc, h_s.c_str(), -1, &hr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(hdc, mm_s.c_str(), -1, &mr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(hdc, ss_s.c_str(), -1, &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    int sep_w = layout.dpi_scale(8);
    int sep1_cx = (hh_cx + mm_cx) / 2;
    int sep2_cx = (mm_cx + ss_cx) / 2;
    RECT sep1r{sep1_cx - sep_w / 2, y + td_off, sep1_cx + sep_w / 2, y + td_off + field_h};
    RECT sep2r{sep2_cx - sep_w / 2, y + td_off, sep2_cx + sep_w / 2, y + td_off + field_h};
    DrawTextW(hdc, L":", -1, &sep1r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(hdc, L":", -1, &sep2r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, ctx.fontSm);
    SetTextColor(hdc, th.text);
    btn(hdc, {hh_cx - abw / 2, y + dn_off, hh_cx + abw / 2, y + dn_off + abh}, false, L"\u25BC",
        tmr_act(i, A_TMR_HDN), ctx);
    btn(hdc, {mm_cx - abw / 2, y + dn_off, mm_cx + abw / 2, y + dn_off + abh}, false, L"\u25BC",
        tmr_act(i, A_TMR_MDN), ctx);
    btn(hdc, {ss_cx - abw / 2, y + dn_off, ss_cx + abw / 2, y + dn_off + abh}, false, L"\u25BC",
        tmr_act(i, A_TMR_SDN), ctx);
}

static void paint_timer_running(HDC hdc, int cw, int y, int i, sc::time_point now, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    auto& th = ctx.theme;
    auto& ts = ctx.app.timers[i];
    int abh = layout.dpi_scale(16);
    int up_off = layout.dpi_scale(4);
    int td_off = up_off + abh + layout.dpi_scale(2);
    int dn_off = td_off + layout.dpi_scale(40) + layout.dpi_scale(2);
    bool expired = ts.t.expired(now);
    COLORREF tcol = expired ? th.expire : (ts.t.remaining(now) < 10s) ? th.warn : th.text;
    std::wstring tstr = format_timer_display(ts.t.remaining(now));

    SelectObject(hdc, ctx.fontSm);
    SetTextColor(hdc, th.dim);
    std::wstring subtitle = ts.label.empty() ? format_timer_edit(duration_cast<Timer::dur>(ts.dur)) : ts.label;
    RECT sr{0, y + up_off, cw, y + up_off + layout.dpi_scale(20)};
    DrawTextW(hdc, subtitle.c_str(), -1, &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, ctx.fontLarge);
    SetTextColor(hdc, tcol);
    RECT tr{0, y + up_off + layout.dpi_scale(20), cw, y + dn_off + abh};
    DrawTextW(hdc, tstr.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void paint_timer_controls(HDC hdc, int cw, int y, int i, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    auto& ts = ctx.app.timers[i];
    int abh = layout.dpi_scale(16), gap = layout.dpi_scale(6), bh = layout.dpi_scale(28);
    int up_off = layout.dpi_scale(4);
    int td_off = up_off + abh + layout.dpi_scale(2);
    int dn_off = td_off + layout.dpi_scale(40) + layout.dpi_scale(2);
    int by_off = dn_off + abh + gap;
    bool running = ts.t.is_running();

    SelectObject(hdc, ctx.fontSm);
    SetTextColor(hdc, ctx.theme.text);
    int cw2 = layout.dpi_scale(86);
    int cx0 = (cw - 2 * cw2 - gap) / 2;
    btn(hdc, {cx0, y + by_off, cx0 + cw2, y + by_off + bh}, running, running ? L"Pause" : L"Start",
        tmr_act(i, A_TMR_START), ctx);
    btn(hdc, {cx0 + cw2 + gap, y + by_off, cx0 + 2 * cw2 + gap, y + by_off + bh}, false, L"Reset",
        tmr_act(i, A_TMR_RST), ctx);

    int pm_sz = layout.dpi_scale(22), pm_margin = layout.dpi_scale(6);
    int pm_top = y + layout.tmr_h - pm_sz;
    if ((int)ctx.app.timers.size() < Config::MAX_TIMERS)
        btn(hdc, {cw - pm_margin - pm_sz, pm_top, cw - pm_margin, pm_top + pm_sz}, false, L"+",
            tmr_act(i, A_TMR_ADD), ctx);
    if ((int)ctx.app.timers.size() > 1)
        btn(hdc, {pm_margin, pm_top, pm_margin + pm_sz, pm_top + pm_sz}, false, L"\u2212",
            tmr_act(i, A_TMR_DEL), ctx);
}

export int paint_timers(HDC hdc, int cw, int y, PaintCtx& ctx, sc::time_point now) {
    for (int i = 0; i < (int)ctx.app.timers.size(); ++i) {
        divider(hdc, y, cw, ctx);
        auto& ts = ctx.app.timers[i];
        if (ts.t.touched()) {
            paint_timer_progress(hdc, cw, y, i, now, ctx);
            paint_timer_running(hdc, cw, y, i, now, ctx);
        } else {
            paint_timer_idle(hdc, cw, y, i, ctx);
        }
        paint_timer_controls(hdc, cw, y, i, ctx);
        y += ctx.layout.tmr_h;
    }
    return y;
}
