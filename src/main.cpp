#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
// Forward-declare DWM to avoid MinGW header chain issues (uxtheme.h missing commctrl.h).
extern "C" __declspec(dllimport) HRESULT __stdcall
DwmSetWindowAttribute(HWND hwnd, DWORD attr, LPCVOID data, DWORD size);
// Forward-declare DPI APIs for MinGW compatibility (Windows 10 1607+/1703+).
extern "C" __declspec(dllimport) UINT __stdcall GetDpiForWindow(HWND hwnd);
extern "C" __declspec(dllimport) BOOL __stdcall
SetProcessDpiAwarenessContext(HANDLE value);
// DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = ((HANDLE)-4)
static const HANDLE DPI_CTX_PER_MONITOR_V2 = ((HANDLE)(LONG_PTR)-4);
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
import config;
import stopwatch;
import timer;

using namespace std::chrono;
using sc = steady_clock;

// ─── RAII wrapper for GDI objects ────────────────────────────────────────────
struct GdiObj {
    HGDIOBJ h;
    explicit GdiObj(HGDIOBJ h) : h(h) {}
    ~GdiObj() { if (h) DeleteObject(h); }
    GdiObj(const GdiObj&) = delete;
    GdiObj& operator=(const GdiObj&) = delete;
    operator HGDIOBJ() const { return h; }
};

// ─── Theme ────────────────────────────────────────────────────────────────────
struct Theme {
    COLORREF bg       = RGB( 26,  26,  26);
    COLORREF bar      = RGB( 35,  35,  38);
    COLORREF btn      = RGB( 40,  40,  44);
    COLORREF active   = RGB( 80,  80,  88);
    COLORREF text     = RGB(204, 204, 204);
    COLORREF dim      = RGB( 90,  90,  90);
    COLORREF warn     = RGB(240, 140,  30);
    COLORREF expire   = RGB(200,  50,  50);
    COLORREF blink    = RGB(110, 110, 118);
    COLORREF fill     = RGB( 38,  38,  50);
    COLORREF fill_exp = RGB( 72,  18,  18);
};
static constexpr Theme theme;

// ─── Blink ────────────────────────────────────────────────────────────────────
constexpr DWORD BLINK_MS = 120;

// ─── DPI scaling ─────────────────────────────────────────────────────────────
static int g_dpi = 96; // default 96 DPI = 100%

static int dpi_scale(int value) {
    return MulDiv(value, g_dpi, 96);
}

// ─── Layout ───────────────────────────────────────────────────────────────────
struct Layout {
    int bar_h  =  36;
    int clk_h  =  62;
    int sw_h   = 100;
    int tmr_h  = 118;
    int btn_h  =  28;

    int w_pin  = 44;
    int w_clk  = 48;
    int w_sw   = 76;
    int w_tmr  = 54;
    int bar_gap = 6;

    int bar_min_client_w() const {
        return w_pin + w_clk + w_sw + w_tmr + 3 * bar_gap + 2 * dpi_scale(8);
    }
};
static Layout layout;

static void update_layout_for_dpi() {
    layout.bar_h   = dpi_scale(36);
    layout.clk_h   = dpi_scale(62);
    layout.sw_h    = dpi_scale(100);
    layout.tmr_h   = dpi_scale(118);
    layout.btn_h   = dpi_scale(28);
    layout.w_pin   = dpi_scale(44);
    layout.w_clk   = dpi_scale(48);
    layout.w_sw    = dpi_scale(76);
    layout.w_tmr   = dpi_scale(54);
    layout.bar_gap = dpi_scale(6);
}

constexpr int MAX_TIMERS = 3;

// ─── Action IDs ───────────────────────────────────────────────────────────────
enum Act {
    A_TOPMOST = 1, A_SHOW_CLK, A_SHOW_SW, A_SHOW_TMR,
    A_SW_START, A_SW_LAP, A_SW_RESET, A_SW_GET,
    A_TMR_BASE = 100,
};
constexpr int TMR_STRIDE  = 8;
constexpr int A_TMR_MUP   = 0;
constexpr int A_TMR_MDN   = 1;
constexpr int A_TMR_SUP   = 2;
constexpr int A_TMR_SDN   = 3;
constexpr int A_TMR_START = 4;
constexpr int A_TMR_RST   = 5;
constexpr int A_TMR_ADD   = 6;
constexpr int A_TMR_DEL   = 7;

static int tmr_act(int i, int off) { return A_TMR_BASE + i * TMR_STRIDE + off; }

// ─── Domain wrappers ──────────────────────────────────────────────────────────
struct TimerSlot {
    Timer   t;
    seconds dur{60s};
};

struct App {
    Stopwatch              sw;
    std::wstring           sw_lap_file;
    std::vector<TimerSlot> timers;
    bool show_clk  = true;
    bool show_sw   = true;
    bool show_tmr  = true;
    bool topmost   = false;
    int  blink_act = 0;
    DWORD blink_t  = 0;
    App() {
        timers.resize(1);
        for (auto& ts : timers) ts.t.set(ts.dur);
    }
};

// ─── Globals ──────────────────────────────────────────────────────────────────
static App   app;
static FILE* g_log_file = nullptr;
static std::vector<std::pair<RECT,int>> g_btns;
static HFONT hFontBig, hFontLarge, hFontSm;
static HWND  g_hwnd;
static int   g_timer_ms = 100;

static void sync_timer(HWND hwnd) {
    int want = (app.show_sw && app.sw.is_running()) ? 20 : 100;
    if (want != g_timer_ms) { g_timer_ms = want; SetTimer(hwnd, 1, want, nullptr); }
}

// ─── Diagnostics ──────────────────────────────────────────────────────────────
static void dbg(const std::wstring& msg) {
    OutputDebugStringW(msg.c_str());
    OutputDebugStringW(L"\n");
    if (g_log_file) { fwprintf(g_log_file, L"%ls\n", msg.c_str()); fflush(g_log_file); }
}

// ─── Config ───────────────────────────────────────────────────────────────────
static std::filesystem::path config_path() {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    return std::filesystem::path{exe}.parent_path() / "config.ini";
}

static void save_config() {
    auto path = config_path();
    dbg(std::format(L"[chrono] save_config: {}", path.wstring()));
    std::ofstream f(path);
    if (!f) { dbg(L"[chrono] save_config: open failed"); return; }
    Config cfg;
    cfg.show_clk   = app.show_clk;
    cfg.show_sw    = app.show_sw;
    cfg.show_tmr   = app.show_tmr;
    cfg.topmost    = app.topmost;
    cfg.num_timers = (int)app.timers.size();
    for (int i = 0; i < (int)app.timers.size(); ++i)
        cfg.timer_secs[i] = (int)app.timers[i].dur.count();
    if (g_hwnd) {
        RECT wr; GetWindowRect(g_hwnd, &wr);
        cfg.pos_valid = true;
        cfg.win_x = wr.left;
        cfg.win_y = wr.top;
        cfg.win_w = wr.right - wr.left;
    }
    config_write(cfg, f);
}

static void load_config(HWND hwnd) {
    auto path = config_path();
    dbg(std::format(L"[chrono] load_config: {}", path.wstring()));
    std::ifstream f(path);
    if (!f) { dbg(L"[chrono] no config, using defaults"); return; }
    Config cfg;
    config_read(cfg, f);
    dbg(std::format(L"[chrono] loaded: clk={} sw={} tmr={} top={} pos={} ({},{} w={})",
                    cfg.show_clk, cfg.show_sw, cfg.show_tmr, cfg.topmost,
                    cfg.pos_valid, cfg.win_x, cfg.win_y, cfg.win_w));
    app.show_clk = cfg.show_clk;
    app.show_sw  = cfg.show_sw;
    app.show_tmr = cfg.show_tmr;
    app.topmost  = cfg.topmost;
    int n = std::min(cfg.num_timers, MAX_TIMERS);
    app.timers.resize(n);
    for (int i = 0; i < n; ++i) {
        app.timers[i].dur = seconds{cfg.timer_secs[i]};
        app.timers[i].t.reset();
        app.timers[i].t.set(app.timers[i].dur);
    }
    if (app.topmost)
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if (cfg.pos_valid) {
        RECT cur; GetWindowRect(hwnd, &cur);
        SetWindowPos(hwnd, nullptr, cfg.win_x, cfg.win_y,
                     cfg.win_w, cur.bottom - cur.top, SWP_NOZORDER);
    }
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
static HFONT make_font(int pt, bool bold) {
    int h = -MulDiv(pt, g_dpi, 72);
    return CreateFontW(h, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                       FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

static void recreate_fonts() {
    if (hFontBig)   DeleteObject(hFontBig);
    if (hFontLarge) DeleteObject(hFontLarge);
    if (hFontSm)    DeleteObject(hFontSm);
    hFontBig   = make_font(26, true);
    hFontLarge = make_font(34, true);
    hFontSm    = make_font(11, false);
    if (!hFontBig)   hFontBig   = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!hFontLarge) hFontLarge = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!hFontSm)    hFontSm   = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

static std::wstring fmt_sw(sc::duration d) {
    auto total_ms = duration_cast<milliseconds>(d).count();
    auto ms      = total_ms % 1000;
    auto total_s = total_ms / 1000;
    auto s = total_s % 60, m = total_s / 60;
    return std::format(L"{:02}:{:02}.{:03}", m, s, ms);
}

static std::wstring fmt_hms(sc::duration d) {
    auto total_s = duration_cast<seconds>(d).count();
    auto s = total_s % 60, m = (total_s / 60) % 60, h = total_s / 3600;
    return std::format(L"{:02}:{:02}:{:02}", h, m, s);
}

static std::wstring fmt_ms(Timer::dur d) {
    auto total_s = duration_cast<seconds>(d).count();
    auto s = total_s % 60, m = total_s / 60;
    return std::format(L"{:02}:{:02}", m, s);
}

// Draw a rounded button, record rect for hit testing, return rect.
static RECT btn(HDC hdc, RECT r, bool active, const wchar_t* label, int id,
                COLORREF override_col = 0) {
    bool blinking = id && app.blink_act == id &&
                    (GetTickCount() - app.blink_t) < BLINK_MS;
    COLORREF col = blinking      ? theme.blink
                 : override_col  ? override_col
                 : (active       ? theme.active : theme.btn);
    GdiObj br{CreateSolidBrush(col)};
    GdiObj pn{CreatePen(PS_NULL, 0, 0)};
    auto*  obr = (HBRUSH)SelectObject(hdc, br);
    auto*  opn = (HPEN)  SelectObject(hdc, pn);
    int rr = dpi_scale(6);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rr, rr);
    SelectObject(hdc, obr); SelectObject(hdc, opn);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme.text);
    DrawTextW(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (id) g_btns.push_back({r, id});
    return r;
}

static void divider(HDC hdc, int y, int cw) {
    GdiObj pn{CreatePen(PS_SOLID, 1, RGB(50, 50, 55))};
    auto* op = (HPEN)SelectObject(hdc, pn);
    MoveToEx(hdc, 0, y, nullptr);
    LineTo(hdc, cw, y);
    SelectObject(hdc, op);
}

static void resize_window(HWND hwnd) {
    int h = layout.bar_h;
    if (app.show_clk) h += layout.clk_h;
    if (app.show_sw)  h += layout.sw_h;
    if (app.show_tmr) h += (int)app.timers.size() * layout.tmr_h;
    RECT wr, cr;
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);
    int nc_h = (wr.bottom - wr.top) - cr.bottom;
    int cur_w = wr.right - wr.left;
    SetWindowPos(hwnd, nullptr, 0, 0,
                 cur_w, h + nc_h,
                 SWP_NOMOVE | SWP_NOZORDER);
}

// ─── Paint ────────────────────────────────────────────────────────────────────
static void paint_all(HDC hdc, int cw) {
    g_btns.clear();
    SetBkMode(hdc, TRANSPARENT);

    RECT all{0, 0, cw, 9999};
    GdiObj bgbr{CreateSolidBrush(theme.bg)};
    FillRect(hdc, &all, (HBRUSH)bgbr.h);

    // ── Top bar ─────────────────────────────────────────────────────────────
    RECT bar{0, 0, cw, layout.bar_h};
    GdiObj barbr{CreateSolidBrush(theme.bar)};
    FillRect(hdc, &bar, (HBRUSH)barbr.h);

    SelectObject(hdc, hFontSm);
    int by = (layout.bar_h - layout.btn_h) / 2;
    int bx = (cw - (layout.w_pin + layout.w_clk + layout.w_sw + layout.w_tmr + 3*layout.bar_gap)) / 2;
    btn(hdc, {bx, by, bx+layout.w_pin, by+layout.btn_h}, app.topmost,  L"Pin",       A_TOPMOST);  bx += layout.w_pin+layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_clk, by+layout.btn_h}, app.show_clk, L"Clock",     A_SHOW_CLK); bx += layout.w_clk+layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_sw,  by+layout.btn_h}, app.show_sw,  L"Stopwatch", A_SHOW_SW);  bx += layout.w_sw +layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_tmr, by+layout.btn_h}, app.show_tmr, L"Timers",    A_SHOW_TMR);

    int y = layout.bar_h;
    auto now = sc::now();

    // ── Clock ────────────────────────────────────────────────────────────────
    if (app.show_clk) {
        divider(hdc, y, cw);
        SYSTEMTIME st;
        GetLocalTime(&st);
        auto buf = std::format(L"{:02}:{:02}:{:02}", st.wHour, st.wMinute, st.wSecond);
        SelectObject(hdc, hFontBig);
        SetTextColor(hdc, theme.text);
        RECT tr{0, y, cw, y + layout.clk_h};
        DrawTextW(hdc, buf.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        y += layout.clk_h;
    }

    // ── Stopwatch ────────────────────────────────────────────────────────────
    if (app.show_sw) {
        divider(hdc, y, cw);
        auto elap = app.sw.elapsed(now);
        std::wstring etime = (elap >= 1h) ? fmt_hms(elap) : fmt_sw(elap);

        SelectObject(hdc, hFontBig);
        SetTextColor(hdc, theme.text);
        RECT tr{0, y + dpi_scale(4), cw, y + dpi_scale(44)};
        DrawTextW(hdc, etime.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hFontSm);
        bool running = app.sw.is_running();
        int  bw = dpi_scale(76), gap = dpi_scale(6), bh = dpi_scale(28);
        int  bx0 = (cw - 3*bw - 2*gap) / 2;
        int  by0 = y + dpi_scale(46);
        btn(hdc, {bx0,            by0, bx0+bw,          by0+bh}, running, running ? L"Stop" : L"Start", A_SW_START);
        btn(hdc, {bx0+bw+gap,     by0, bx0+2*bw+gap,    by0+bh}, false,   L"Lap",                       A_SW_LAP);
        btn(hdc, {bx0+2*(bw+gap), by0, bx0+3*bw+2*gap,  by0+bh}, false,   L"Reset",                     A_SW_RESET);

        bool has_file = !app.sw_lap_file.empty();
        int  gbw = dpi_scale(100), gbh = dpi_scale(18);
        btn(hdc, {(cw-gbw)/2, by0+bh+dpi_scale(4), (cw+gbw)/2, by0+bh+dpi_scale(4)+gbh},
            false, L"Get Laps", has_file ? A_SW_GET : 0,
            has_file ? theme.btn : theme.dim);
        y += layout.sw_h;
    }

    // ── Timers ───────────────────────────────────────────────────────────────
    if (app.show_tmr) {
        int abw = dpi_scale(34), abh = dpi_scale(16), gap = dpi_scale(6), bh = dpi_scale(28);
        int up_off = dpi_scale(4);
        int td_off = up_off + abh + dpi_scale(2);
        int dn_off = td_off + dpi_scale(40) + dpi_scale(2);
        int by_off = dn_off + abh + gap;

        for (int i = 0; i < (int)app.timers.size(); ++i) {
            divider(hdc, y, cw);
            auto& ts      = app.timers[i];
            bool  running = ts.t.is_running();
            bool  touched = ts.t.touched();
            bool  expired = touched && ts.t.expired(now);

            if (touched) {
                COLORREF fillcol = expired ? theme.fill_exp : theme.fill;
                int fw = cw;
                if (!expired) {
                    auto total = duration_cast<microseconds>(ts.dur).count();
                    auto rem   = duration_cast<microseconds>(ts.t.remaining(now)).count();
                    fw = total > 0 ? (int)(cw * (double)(total - rem) / total) : 0;
                }
                RECT fr{0, y, fw, y + layout.tmr_h};
                GdiObj fbr{CreateSolidBrush(fillcol)};
                FillRect(hdc, &fr, (HBRUSH)fbr.h);
            }

            int mm_cx = cw/2 - dpi_scale(34), ss_cx = cw/2 + dpi_scale(34);

            SelectObject(hdc, hFontSm);
            if (!touched) {
                btn(hdc, {mm_cx-abw/2, y+up_off, mm_cx+abw/2, y+up_off+abh}, false, L"▲", tmr_act(i, A_TMR_MUP));
                btn(hdc, {ss_cx-abw/2, y+up_off, ss_cx+abw/2, y+up_off+abh}, false, L"▲", tmr_act(i, A_TMR_SUP));
            }

            std::wstring tstr = touched ? fmt_ms(ts.t.remaining(now))
                                        : fmt_ms(duration_cast<Timer::dur>(ts.dur));
            COLORREF tcol = expired ? theme.expire
                : (touched && ts.t.remaining(now) < 10s) ? theme.warn
                : theme.text;
            if (touched) {
                SelectObject(hdc, hFontSm);
                SetTextColor(hdc, theme.dim);
                std::wstring sstr = fmt_ms(duration_cast<Timer::dur>(ts.dur));
                RECT sr{0, y + up_off, cw, y + up_off + dpi_scale(20)};
                DrawTextW(hdc, sstr.c_str(), -1, &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, hFontLarge);
                SetTextColor(hdc, tcol);
                RECT tr{0, y + up_off + dpi_scale(20), cw, y + dn_off + abh};
                DrawTextW(hdc, tstr.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else {
                SelectObject(hdc, hFontBig);
                SetTextColor(hdc, tcol);
                RECT tr{0, y + td_off, cw, y + td_off + dpi_scale(40)};
                DrawTextW(hdc, tstr.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            SelectObject(hdc, hFontSm);
            SetTextColor(hdc, theme.text);
            if (!touched) {
                btn(hdc, {mm_cx-abw/2, y+dn_off, mm_cx+abw/2, y+dn_off+abh}, false, L"▼", tmr_act(i, A_TMR_MDN));
                btn(hdc, {ss_cx-abw/2, y+dn_off, ss_cx+abw/2, y+dn_off+abh}, false, L"▼", tmr_act(i, A_TMR_SDN));
            }

            int cw2 = dpi_scale(86);
            int cx0 = (cw - 2*cw2 - gap) / 2;
            btn(hdc, {cx0,         y+by_off, cx0+cw2,       y+by_off+bh}, running,
                running ? L"Pause" : L"Start", tmr_act(i, A_TMR_START));
            btn(hdc, {cx0+cw2+gap, y+by_off, cx0+2*cw2+gap, y+by_off+bh}, false, L"Reset",
                tmr_act(i, A_TMR_RST));

            int pm_sz = dpi_scale(22), pm_margin = dpi_scale(6);
            int pm_top = y + layout.tmr_h - pm_sz - dpi_scale(4);
            if ((int)app.timers.size() < MAX_TIMERS)
                btn(hdc, {cw-pm_margin-pm_sz, pm_top, cw-pm_margin, pm_top+pm_sz},
                    false, L"+", tmr_act(i, A_TMR_ADD));
            if ((int)app.timers.size() > 1)
                btn(hdc, {pm_margin, pm_top, pm_margin+pm_sz, pm_top+pm_sz},
                    false, L"−", tmr_act(i, A_TMR_DEL));

            y += layout.tmr_h;
        }
    }
}

// ─── Actions ──────────────────────────────────────────────────────────────────
static bool wants_blink(int act) {
    switch (act) {
    case A_TOPMOST: case A_SHOW_CLK: case A_SHOW_SW: case A_SHOW_TMR:
    case A_SW_START:
        return false;
    default:
        if (act >= A_TMR_BASE) {
            int off = (act - A_TMR_BASE) % TMR_STRIDE;
            if (off == A_TMR_START || off == A_TMR_ADD || off == A_TMR_DEL)
                return false;
        }
        return true;
    }
}

static void handle(HWND hwnd, int act) {
    auto now = sc::now();
    if (wants_blink(act)) { app.blink_act = act; app.blink_t = GetTickCount(); }

    bool do_save = false;

    switch (act) {
    case A_TOPMOST:
        app.topmost = !app.topmost;
        SetWindowPos(hwnd, app.topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        do_save = true;
        break;
    case A_SHOW_CLK: app.show_clk = !app.show_clk; resize_window(hwnd); do_save = true; break;
    case A_SHOW_SW:  app.show_sw  = !app.show_sw;  resize_window(hwnd); do_save = true; break;
    case A_SHOW_TMR: app.show_tmr = !app.show_tmr; resize_window(hwnd); do_save = true; break;
    case A_SW_START:
        if (!app.sw.is_running()) {
            if (app.sw_lap_file.empty()) {
                SYSTEMTIME st; GetLocalTime(&st);
                app.sw_lap_file = std::format(L"stopwatch-{:04}{:02}{:02}-{:02}{:02}{:02}-{:03}.txt",
                                              st.wYear, st.wMonth, st.wDay,
                                              st.wHour, st.wMinute, st.wSecond,
                                              st.wMilliseconds);
            }
            app.sw.start(now);
        } else {
            app.sw.stop(now);
        }
        break;
    case A_SW_LAP:
        if (app.sw.is_running()) {
            app.sw.lap(now);
            if (!app.sw_lap_file.empty()) {
                FILE* f = _wfopen(app.sw_lap_file.c_str(), L"a");
                if (f) {
                    const auto& laps = app.sw.laps();
                    auto n = laps.size();
                    sc::duration cum{};
                    for (auto& l : laps) cum += l;
                    fwprintf(f, L"Lap %-3zu   split %-14ls   total %ls\n",
                             n, fmt_sw(laps.back()).c_str(), fmt_sw(cum).c_str());
                    fclose(f);
                } else {
                    MessageBoxW(hwnd, L"Could not write lap file.\nLap data may be lost.",
                                L"Chronos", MB_OK | MB_ICONWARNING);
                }
            }
        }
        break;
    case A_SW_RESET:
        app.sw.reset();
        app.sw_lap_file.clear();
        break;
    case A_SW_GET:
        if (!app.sw_lap_file.empty())
            ShellExecuteW(nullptr, L"open", app.sw_lap_file.c_str(),
                          nullptr, nullptr, SW_SHOW);
        break;
    default:
        if (act >= A_TMR_BASE) {
            int idx = (act - A_TMR_BASE) / TMR_STRIDE;
            int off = (act - A_TMR_BASE) % TMR_STRIDE;
            if (idx < 0 || idx >= (int)app.timers.size()) break;
            auto& ts = app.timers[idx];
            if (off == A_TMR_START) {
                if (!ts.t.touched())          ts.t.start(now);
                else if (ts.t.is_running())   ts.t.pause(now);
                else                          ts.t.start(now);
            } else if (off == A_TMR_RST) {
                ts.t.reset(); ts.t.set(ts.dur);
            } else if (off == A_TMR_ADD) {
                if ((int)app.timers.size() < MAX_TIMERS) {
                    TimerSlot ns; ns.t.set(ns.dur);
                    app.timers.insert(app.timers.begin() + idx + 1, ns);
                    resize_window(hwnd); do_save = true;
                }
            } else if (off == A_TMR_DEL) {
                if ((int)app.timers.size() > 1) {
                    app.timers.erase(app.timers.begin() + idx);
                    resize_window(hwnd); do_save = true;
                }
            } else if (!ts.t.touched()) {
                auto adj = [&](int ds) {
                    auto new_s = ts.dur.count() + ds;
                    ts.dur = seconds{std::clamp(new_s, 10LL, 86400LL)};
                    ts.t.reset(); ts.t.set(ts.dur);
                };
                if (off == A_TMR_MUP) { adj(+60); do_save = true; }
                if (off == A_TMR_MDN) { adj(-60); do_save = true; }
                if (off == A_TMR_SUP) { adj(+10); do_save = true; }
                if (off == A_TMR_SDN) { adj(-10); do_save = true; }
            }
        }
    }
    if (do_save) save_config();
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd);
}

// ─── Theme detection ─────────────────────────────────────────────────────────
static bool system_prefers_dark() {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD val = 0, size = sizeof(val);
        bool ok = RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr,
                                   (LPBYTE)&val, &size) == ERROR_SUCCESS;
        RegCloseKey(key);
        if (ok) return val == 0;
    }
    return true; // default to dark
}

// ─── WndProc ──────────────────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        // Get actual DPI for this window's monitor
        UINT wdpi = GetDpiForWindow(hwnd);
        if (wdpi != 0) g_dpi = (int)wdpi;
        update_layout_for_dpi();
        recreate_fonts();
        SetTimer(hwnd, 1, 100, nullptr);
        BOOL dark = system_prefers_dark() ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */,
                              &dark, sizeof(dark));
        load_config(hwnd);
        resize_window(hwnd);
        return 0;
    }
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        sync_timer(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT cr; GetClientRect(hwnd, &cr);
        HDC     mdc = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, cr.right, cr.bottom);
        auto*   old = SelectObject(mdc, bmp);
        paint_all(mdc, cr.right);
        BitBlt(hdc, 0, 0, cr.right, cr.bottom, mdc, 0, 0, SRCCOPY);
        SelectObject(mdc, old);
        DeleteObject(bmp); DeleteDC(mdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        for (auto& [r, id] : g_btns)
            if (PtInRect(&r, pt)) { handle(hwnd, id); break; }
        return 0;
    }
    case WM_KEYDOWN: {
        switch (wp) {
        case VK_SPACE:
            // Start/stop stopwatch if visible, else first timer
            if (app.show_sw)
                handle(hwnd, A_SW_START);
            else if (app.show_tmr && !app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_START));
            break;
        case 'L':
            if (app.show_sw) handle(hwnd, A_SW_LAP);
            break;
        case 'R':
            if (app.show_sw)
                handle(hwnd, A_SW_RESET);
            else if (app.show_tmr && !app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_RST));
            break;
        case 'T':
            handle(hwnd, A_TOPMOST);
            break;
        case '1': case '2': case '3': {
            int idx = (int)(wp - '1');
            if (app.show_tmr && idx < (int)app.timers.size())
                handle(hwnd, tmr_act(idx, A_TMR_START));
            break;
        }
        }
        return 0;
    }
    case WM_SIZING: {
        RECT wr, cr;
        GetWindowRect(hwnd, &wr); GetClientRect(hwnd, &cr);
        int nc_h = (wr.bottom - wr.top) - cr.bottom;
        int h = layout.bar_h;
        if (app.show_clk) h += layout.clk_h;
        if (app.show_sw)  h += layout.sw_h;
        if (app.show_tmr) h += (int)app.timers.size() * layout.tmr_h;
        int want_h = h + nc_h;
        auto* r = (RECT*)lp;
        if (wp == WMSZ_TOP || wp == WMSZ_TOPLEFT || wp == WMSZ_TOPRIGHT)
            r->top = r->bottom - want_h;
        else
            r->bottom = r->top + want_h;
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
        auto* m = (MINMAXINFO*)lp;
        DWORD ws = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
        RECT adj{0, 0, layout.bar_min_client_w(), 0};
        AdjustWindowRectEx(&adj, ws, FALSE, 0);
        m->ptMinTrackSize.x = adj.right - adj.left;
        int ch = layout.bar_h;
        if (app.show_clk) ch += layout.clk_h;
        if (app.show_sw)  ch += layout.sw_h;
        if (app.show_tmr) ch += (int)app.timers.size() * layout.tmr_h;
        RECT adj_h{0, 0, 0, ch};
        AdjustWindowRectEx(&adj_h, ws, FALSE, 0);
        m->ptMinTrackSize.y = adj_h.bottom - adj_h.top;
        return 0;
    }
    case WM_DPICHANGED: {
        g_dpi = HIWORD(wp);
        update_layout_for_dpi();
        recreate_fonts();
        // Windows provides the suggested new window rect
        auto* suggested = (RECT*)lp;
        SetWindowPos(hwnd, nullptr,
                     suggested->left, suggested->top,
                     suggested->right - suggested->left,
                     suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        resize_window(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_EXITSIZEMOVE:
        save_config();
        return 0;
    case WM_DESTROY:
        save_config();
        KillTimer(hwnd, 1);
        DeleteObject(hFontBig); DeleteObject(hFontLarge); DeleteObject(hFontSm);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ─── WinMain ──────────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--debug") == 0) {
            wchar_t exe[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, exe, MAX_PATH);
            auto log_path = std::filesystem::path{exe}.parent_path() / "debug.log";
            g_log_file = _wfopen(log_path.c_str(), L"a");
            if (g_log_file) fprintf(g_log_file, "\n=== chronos start ===\n");
        }
    }
    LocalFree(argv);

    // Set per-monitor DPI awareness v2
    SetProcessDpiAwarenessContext(DPI_CTX_PER_MONITOR_V2);

    // Get initial DPI from primary monitor before window exists
    HDC dc = GetDC(nullptr);
    g_dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(nullptr, dc);
    update_layout_for_dpi();

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ChronoApp";
    RegisterClassExW(&wc);

    constexpr DWORD ws = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
    int init_h = layout.bar_h + layout.clk_h + layout.sw_h + (int)app.timers.size() * layout.tmr_h;
    RECT wr{0, 0, layout.bar_min_client_w(), init_h};
    AdjustWindowRect(&wr, ws, FALSE);

    HWND hwnd = CreateWindowExW(
        0, L"ChronoApp", L"Chrono", ws,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (g_log_file) { fprintf(g_log_file, "=== chronos exit ===\n"); fclose(g_log_file); }
    return (int)msg.wParam;
}
