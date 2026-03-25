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
export module painting;
import actions;
import app;
import config;
import formatting;
import layout;
import timer;

using namespace std::chrono;
using sc = steady_clock;

// ─── RAII wrapper for GDI objects ────────────────────────────────────────────
export struct GdiObj {
    HGDIOBJ h;
    explicit GdiObj(HGDIOBJ h) : h(h) {}
    ~GdiObj() { if (h) DeleteObject(h); }
    GdiObj(const GdiObj&) = delete;
    GdiObj& operator=(const GdiObj&) = delete;
    operator HGDIOBJ() const { return h; }
};

// ─── Theme ────────────────────────────────────────────────────────────────────
export struct Theme {
    COLORREF bg;
    COLORREF bar;
    COLORREF btn;
    COLORREF active;
    COLORREF text;
    COLORREF dim;
    COLORREF warn;
    COLORREF expire;
    COLORREF blink;
    COLORREF fill;
    COLORREF fill_exp;
    COLORREF help_bg;
    COLORREF divider;
};

export constexpr Theme dark_theme{
    .bg       = RGB( 26,  26,  26),
    .bar      = RGB( 35,  35,  38),
    .btn      = RGB( 40,  40,  44),
    .active   = RGB( 80,  80,  88),
    .text     = RGB(204, 204, 204),
    .dim      = RGB( 90,  90,  90),
    .warn     = RGB(240, 140,  30),
    .expire   = RGB(200,  50,  50),
    .blink    = RGB(110, 110, 118),
    .fill     = RGB( 38,  38,  50),
    .fill_exp = RGB( 72,  18,  18),
    .help_bg  = RGB( 20,  20,  20),
    .divider  = RGB( 50,  50,  55),
};

export constexpr Theme light_theme{
    .bg       = RGB(243, 243, 243),
    .bar      = RGB(228, 228, 232),
    .btn      = RGB(214, 214, 220),
    .active   = RGB(175, 175, 188),
    .text     = RGB( 28,  28,  28),
    .dim      = RGB(138, 138, 138),
    .warn     = RGB(196,  90,   0),
    .expire   = RGB(196,  36,  36),
    .blink    = RGB(168, 168, 178),
    .fill     = RGB(208, 208, 230),
    .fill_exp = RGB(240, 196, 196),
    .help_bg  = RGB(232, 232, 232),
    .divider  = RGB(198, 198, 208),
};

// ─── Blink duration ──────────────────────────────────────────────────────────
export constexpr auto BLINK_DUR = std::chrono::milliseconds{120};

// ─── Paint context ───────────────────────────────────────────────────────────
export struct PaintCtx {
    App&    app;
    Layout& layout;
    const Theme& theme;
    std::vector<std::pair<RECT,int>>& btns;
    HFONT  fontBig;
    HFONT  fontLarge;
    HFONT  fontSm;
    HBRUSH brBg;
    HBRUSH brBar;
    HBRUSH brBtn;
    HBRUSH brActive;
    HBRUSH brBlink;
    HBRUSH brFill;
    HBRUSH brFillExp;
    HBRUSH brHelp;
    HPEN   pnNull;
    HPEN   pnDivider;
};

// ─── Helpers ─────────────────────────────────────────────────────────────────
static RECT btn(HDC hdc, RECT r, bool active, const wchar_t* label, int id,
                PaintCtx& ctx, std::optional<COLORREF> override_col = std::nullopt) {
    auto& layout = ctx.layout;
    bool blinking = id && ctx.app.blink_act == id &&
                    (sc::now() - ctx.app.blink_t) < BLINK_DUR;
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
    auto*  obr = (HBRUSH)SelectObject(hdc, brush);
    auto*  opn = (HPEN)  SelectObject(hdc, ctx.pnNull);
    int rr = layout.dpi_scale(6);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rr, rr);
    SelectObject(hdc, obr); SelectObject(hdc, opn);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ctx.theme.text);
    DrawTextW(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (id) ctx.btns.push_back({r, id});
    return r;
}

static void divider(HDC hdc, int y, int cw, const PaintCtx& ctx) {
    auto* op = (HPEN)SelectObject(hdc, ctx.pnDivider);
    MoveToEx(hdc, 0, y, nullptr);
    LineTo(hdc, cw, y);
    SelectObject(hdc, op);
}

// ─── Paint sub-functions ──────────────────────────────────────────────────────
static int paint_bar(HDC hdc, int cw, int y, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    RECT bar{0, 0, cw, layout.bar_h};
    FillRect(hdc, &bar, ctx.brBar);

    SelectObject(hdc, ctx.fontSm);
    int by = (layout.bar_h - layout.btn_h) / 2;
    int bx = (cw - (layout.w_pin + layout.w_clk + layout.w_sw + layout.w_tmr + 3*layout.bar_gap)) / 2;
    btn(hdc, {bx, by, bx+layout.w_pin, by+layout.btn_h}, ctx.app.topmost,  L"Pin",       A_TOPMOST, ctx);  bx += layout.w_pin+layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_clk, by+layout.btn_h}, ctx.app.show_clk, L"Clock",     A_SHOW_CLK, ctx); bx += layout.w_clk+layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_sw,  by+layout.btn_h}, ctx.app.show_sw,  L"Stopwatch", A_SHOW_SW, ctx);  bx += layout.w_sw +layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_tmr, by+layout.btn_h}, ctx.app.show_tmr, L"Timers",    A_SHOW_TMR, ctx);
    return layout.bar_h;
}

static int paint_clock(HDC hdc, int cw, int y, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    divider(hdc, y, cw, ctx);
    SYSTEMTIME st;
    GetLocalTime(&st);
    auto buf = std::format(L"{:02}:{:02}:{:02}", st.wHour, st.wMinute, st.wSecond);
    SelectObject(hdc, ctx.fontBig);
    SetTextColor(hdc, ctx.theme.text);
    RECT tr{0, y, cw, y + layout.clk_h};
    DrawTextW(hdc, buf.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return y + layout.clk_h;
}

static int paint_stopwatch(HDC hdc, int cw, int y, PaintCtx& ctx, sc::time_point now) {
    auto& layout = ctx.layout;
    auto& th = ctx.theme;
    divider(hdc, y, cw, ctx);
    auto elap = ctx.app.sw.elapsed(now);
    std::wstring etime = format_stopwatch_display(elap);

    SelectObject(hdc, ctx.fontBig);
    SetTextColor(hdc, th.text);
    RECT tr{0, y + layout.dpi_scale(4), cw, y + layout.dpi_scale(44)};
    DrawTextW(hdc, etime.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, ctx.fontSm);
    bool running = ctx.app.sw.is_running();
    int  bw = layout.dpi_scale(76), gap = layout.dpi_scale(6), bh = layout.dpi_scale(28);
    int  bx0 = (cw - 3*bw - 2*gap) / 2;
    int  by0 = y + layout.dpi_scale(46);
    btn(hdc, {bx0,            by0, bx0+bw,          by0+bh}, running, running ? L"Stop" : L"Start", A_SW_START, ctx);
    btn(hdc, {bx0+bw+gap,     by0, bx0+2*bw+gap,    by0+bh}, false,   L"Lap",                       A_SW_LAP, ctx);
    btn(hdc, {bx0+2*(bw+gap), by0, bx0+3*bw+2*gap,  by0+bh}, false,   L"Reset",                     A_SW_RESET, ctx);

    auto& laps = ctx.app.sw.laps();
    if (!laps.empty()) {
        auto info = std::format(L"Lap {}  \u2014  {}", laps.size(),
                                format_stopwatch_short(laps.back()));
        SetTextColor(hdc, th.dim);
        RECT ir{0, by0+bh+layout.dpi_scale(4), cw, by0+bh+layout.dpi_scale(22)};
        DrawTextW(hdc, info.c_str(), -1, &ir, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    bool has_file = !ctx.app.sw_lap_file.empty();
    int  gbw = layout.dpi_scale(100), gbh = layout.dpi_scale(18);
    auto lap_label = ctx.app.lap_write_failed ? L"Get Laps (!)" : L"Get Laps";
    auto lap_col = ctx.app.lap_write_failed ? th.expire
                 : has_file ? th.btn : th.dim;
    btn(hdc, {(cw-gbw)/2, by0+bh+layout.dpi_scale(24), (cw+gbw)/2, by0+bh+layout.dpi_scale(24)+gbh},
        false, lap_label, has_file ? A_SW_GET : 0, ctx, lap_col);
    return y + layout.sw_h;
}

static int paint_timers(HDC hdc, int cw, int y, PaintCtx& ctx, sc::time_point now) {
    auto& layout = ctx.layout;
    auto& th = ctx.theme;
    int abw = layout.dpi_scale(34), abh = layout.dpi_scale(16), gap = layout.dpi_scale(6), bh = layout.dpi_scale(28);
    int up_off = layout.dpi_scale(4);
    int td_off = up_off + abh + layout.dpi_scale(2);
    int dn_off = td_off + layout.dpi_scale(40) + layout.dpi_scale(2);
    int by_off = dn_off + abh + gap;

    for (int i = 0; i < (int)ctx.app.timers.size(); ++i) {
        divider(hdc, y, cw, ctx);
        auto& ts      = ctx.app.timers[i];
        bool  running = ts.t.is_running();
        bool  touched = ts.t.touched();
        bool  expired = touched && ts.t.expired(now);

        if (touched) {
            HBRUSH fillbr = expired ? ctx.brFillExp : ctx.brFill;
            int fw = cw;
            if (!expired) {
                auto total = duration_cast<microseconds>(ts.dur).count();
                auto rem   = duration_cast<microseconds>(ts.t.remaining(now)).count();
                fw = total > 0 ? (int)(cw * (double)(total - rem) / total) : 0;
            }
            RECT fr{0, y, fw, y + layout.tmr_h};
            FillRect(hdc, &fr, fillbr);
        }

        int col_gap = layout.dpi_scale(52);
        int hh_cx = cw/2 - col_gap, mm_cx = cw/2, ss_cx = cw/2 + col_gap;

        SelectObject(hdc, ctx.fontSm);
        if (!touched) {
            btn(hdc, {hh_cx-abw/2, y+up_off, hh_cx+abw/2, y+up_off+abh}, false, L"\u25B2", tmr_act(i, A_TMR_HUP), ctx);
            btn(hdc, {mm_cx-abw/2, y+up_off, mm_cx+abw/2, y+up_off+abh}, false, L"\u25B2", tmr_act(i, A_TMR_MUP), ctx);
            btn(hdc, {ss_cx-abw/2, y+up_off, ss_cx+abw/2, y+up_off+abh}, false, L"\u25B2", tmr_act(i, A_TMR_SUP), ctx);
        }

        std::wstring tstr = touched ? format_timer_display(ts.t.remaining(now))
                                    : format_timer_edit(duration_cast<Timer::dur>(ts.dur));
        COLORREF tcol = expired ? th.expire
            : (touched && ts.t.remaining(now) < 10s) ? th.warn
            : th.text;
        if (touched) {
            SelectObject(hdc, ctx.fontSm);
            SetTextColor(hdc, th.dim);
            // Show label or original duration as subtitle
            std::wstring subtitle = ts.label.empty()
                ? format_timer_edit(duration_cast<Timer::dur>(ts.dur))
                : ts.label;
            RECT sr{0, y + up_off, cw, y + up_off + layout.dpi_scale(20)};
            DrawTextW(hdc, subtitle.c_str(), -1, &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, ctx.fontLarge);
            SetTextColor(hdc, tcol);
            RECT tr2{0, y + up_off + layout.dpi_scale(20), cw, y + dn_off + abh};
            DrawTextW(hdc, tstr.c_str(), -1, &tr2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            // Show label as dim text above the duration when idle
            if (!ts.label.empty()) {
                SelectObject(hdc, ctx.fontSm);
                SetTextColor(hdc, th.dim);
                RECT lr{0, y + layout.dpi_scale(2), cw, y + up_off + abh};
                DrawTextW(hdc, ts.label.c_str(), -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            SelectObject(hdc, ctx.fontBig);
            SetTextColor(hdc, tcol);
            // Draw each field (H, MM, SS) centered over its column so arrows align
            int field_h = layout.dpi_scale(40);
            int field_half = layout.dpi_scale(22);
            long long total_s_edit = ts.dur.count();
            auto h_s  = std::format(L"{}", total_s_edit / 3600);
            auto mm_s = std::format(L"{:02}", (total_s_edit / 60) % 60);
            auto ss_s = std::format(L"{:02}", total_s_edit % 60);
            RECT hr{hh_cx - field_half, y + td_off, hh_cx + field_half, y + td_off + field_h};
            RECT mr{mm_cx - field_half, y + td_off, mm_cx + field_half, y + td_off + field_h};
            RECT sr{ss_cx - field_half, y + td_off, ss_cx + field_half, y + td_off + field_h};
            DrawTextW(hdc, h_s.c_str(),  -1, &hr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DrawTextW(hdc, mm_s.c_str(), -1, &mr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DrawTextW(hdc, ss_s.c_str(), -1, &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            int sep_w = layout.dpi_scale(8);
            int sep1_cx = (hh_cx + mm_cx) / 2;
            int sep2_cx = (mm_cx + ss_cx) / 2;
            RECT sep1r{sep1_cx - sep_w/2, y + td_off, sep1_cx + sep_w/2, y + td_off + field_h};
            RECT sep2r{sep2_cx - sep_w/2, y + td_off, sep2_cx + sep_w/2, y + td_off + field_h};
            DrawTextW(hdc, L":", -1, &sep1r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DrawTextW(hdc, L":", -1, &sep2r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        SelectObject(hdc, ctx.fontSm);
        SetTextColor(hdc, th.text);
        if (!touched) {
            btn(hdc, {hh_cx-abw/2, y+dn_off, hh_cx+abw/2, y+dn_off+abh}, false, L"\u25BC", tmr_act(i, A_TMR_HDN), ctx);
            btn(hdc, {mm_cx-abw/2, y+dn_off, mm_cx+abw/2, y+dn_off+abh}, false, L"\u25BC", tmr_act(i, A_TMR_MDN), ctx);
            btn(hdc, {ss_cx-abw/2, y+dn_off, ss_cx+abw/2, y+dn_off+abh}, false, L"\u25BC", tmr_act(i, A_TMR_SDN), ctx);
        }

        int cw2 = layout.dpi_scale(86);
        int cx0 = (cw - 2*cw2 - gap) / 2;
        btn(hdc, {cx0,         y+by_off, cx0+cw2,       y+by_off+bh}, running,
            running ? L"Pause" : L"Start", tmr_act(i, A_TMR_START), ctx);
        btn(hdc, {cx0+cw2+gap, y+by_off, cx0+2*cw2+gap, y+by_off+bh}, false, L"Reset",
            tmr_act(i, A_TMR_RST), ctx);

        int pm_sz = layout.dpi_scale(22), pm_margin = layout.dpi_scale(6);
        int pm_top = y + layout.tmr_h - pm_sz;
        if ((int)ctx.app.timers.size() < Config::MAX_TIMERS)
            btn(hdc, {cw-pm_margin-pm_sz, pm_top, cw-pm_margin, pm_top+pm_sz},
                false, L"+", tmr_act(i, A_TMR_ADD), ctx);
        if ((int)ctx.app.timers.size() > 1)
            btn(hdc, {pm_margin, pm_top, pm_margin+pm_sz, pm_top+pm_sz},
                false, L"\u2212", tmr_act(i, A_TMR_DEL), ctx);

        y += layout.tmr_h;
    }
    return y;
}

static void paint_help(HDC hdc, int cw, int y_bottom, PaintCtx& ctx) {
    auto& layout = ctx.layout;
    RECT cr{0, layout.bar_h, cw, y_bottom > layout.bar_h ? y_bottom : layout.bar_h + layout.dpi_scale(200)};
    FillRect(hdc, &cr, ctx.brHelp);

    SelectObject(hdc, ctx.fontSm);
    SetTextColor(hdc, ctx.theme.text);

    struct { const wchar_t* key; const wchar_t* desc; } shortcuts[] = {
        {L"Space", L"Start/Stop stopwatch or first timer"},
        {L"L",     L"Record lap"},
        {L"R",     L"Reset stopwatch or first timer"},
        {L"T",     L"Toggle always-on-top"},
        {L"1-3",   L"Start/Stop timer 1-3"},
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
export void paint_all(HDC hdc, int cw, int ch, PaintCtx& ctx) {
    ctx.btns.clear();
    SetBkMode(hdc, TRANSPARENT);

    RECT all{0, 0, cw, ch};
    FillRect(hdc, &all, ctx.brBg);

    auto now = sc::now();
    int y = paint_bar(hdc, cw, 0, ctx);
    if (ctx.app.show_clk) y = paint_clock(hdc, cw, y, ctx);
    if (ctx.app.show_sw)  y = paint_stopwatch(hdc, cw, y, ctx, now);
    if (ctx.app.show_tmr) y = paint_timers(hdc, cw, y, ctx, now);
    if (ctx.app.show_help) paint_help(hdc, cw, y, ctx);
}
