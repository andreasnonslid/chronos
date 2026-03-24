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
// DPI APIs loaded dynamically to avoid hard dependency on Windows 10 1607+/1703+.
using GetDpiForWindow_t = UINT (WINAPI *)(HWND);
using SetProcessDpiAwarenessContext_t = BOOL (WINAPI *)(HANDLE);
static GetDpiForWindow_t pGetDpiForWindow = nullptr;
static SetProcessDpiAwarenessContext_t pSetProcessDpiAwarenessContext = nullptr;
static const HANDLE DPI_CTX_PER_MONITOR_V2 = ((HANDLE)(LONG_PTR)-4);
static void load_dpi_apis() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        pGetDpiForWindow = (GetDpiForWindow_t)GetProcAddress(user32, "GetDpiForWindow");
        pSetProcessDpiAwarenessContext = (SetProcessDpiAwarenessContext_t)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    }
}
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <cstdio>
import config;
import formatting;
import layout;
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

// ─── Named constants ─────────────────────────────────────────────────────────
constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR = 20;
constexpr int POLL_STOPWATCH_MS = 20;
constexpr int POLL_TIMER_MS    = 100;
constexpr int POLL_IDLE_MS     = 1000;
constexpr int STANDARD_DPI     = 96;
constexpr long long TIMER_MIN_SECS = 0;
constexpr long long TIMER_MAX_SECS = 86400;

// ─── Blink ────────────────────────────────────────────────────────────────────
constexpr auto BLINK_DUR = std::chrono::milliseconds{120};

// Use Config::MAX_TIMERS everywhere (defined in config.ixx)

// ─── Action IDs ───────────────────────────────────────────────────────────────
enum Act {
    A_TOPMOST = 1, A_SHOW_CLK, A_SHOW_SW, A_SHOW_TMR,
    A_SW_START, A_SW_LAP, A_SW_RESET, A_SW_GET,
    A_TMR_BASE = 100,
};
constexpr int TMR_STRIDE  = 10;
constexpr int A_TMR_HUP   = 0;
constexpr int A_TMR_HDN   = 1;
constexpr int A_TMR_MUP   = 2;
constexpr int A_TMR_MDN   = 3;
constexpr int A_TMR_SUP   = 4;
constexpr int A_TMR_SDN   = 5;
constexpr int A_TMR_START = 6;
constexpr int A_TMR_RST   = 7;
constexpr int A_TMR_ADD   = 8;
constexpr int A_TMR_DEL   = 9;

static int tmr_act(int i, int off) { return A_TMR_BASE + i * TMR_STRIDE + off; }

// ─── Domain wrappers ──────────────────────────────────────────────────────────
struct TimerSlot {
    Timer   t;
    seconds dur{60s};
    bool    notified = false;  // true once expiry alert has fired
};

struct App {
    Stopwatch              sw;
    std::wstring           sw_lap_file;
    std::vector<TimerSlot> timers;
    bool show_clk  = true;
    bool show_sw   = true;
    bool show_tmr  = true;
    bool topmost   = false;
    bool show_help = false;
    bool lap_write_failed = false;
    int  blink_act = 0;
    sc::time_point blink_t{};
    App() {
        timers.resize(1);
        for (auto& ts : timers) ts.t.set(ts.dur);
    }
};

// ─── Per-window state (stored via GWLP_USERDATA) ─────────────────────────────
struct WndState {
    App    app;
    Layout layout;
    std::vector<std::pair<RECT,int>> btns;
    HFONT  hFontBig   = nullptr;
    HFONT  hFontLarge = nullptr;
    HFONT  hFontSm    = nullptr;
    bool   fonts_custom = false;
    int    timer_ms   = 100;

    // Pre-created GDI objects for common colors (avoid churn in paint loop)
    HBRUSH brBg      = nullptr;
    HBRUSH brBar     = nullptr;
    HBRUSH brBtn     = nullptr;
    HBRUSH brActive  = nullptr;
    HBRUSH brBlink   = nullptr;
    HBRUSH brFill    = nullptr;
    HBRUSH brFillExp = nullptr;
    HBRUSH brHelp    = nullptr;
    HPEN   pnNull    = nullptr;
    HPEN   pnDivider = nullptr;

    void create_brushes() {
        brBg      = CreateSolidBrush(theme.bg);
        brBar     = CreateSolidBrush(theme.bar);
        brBtn     = CreateSolidBrush(theme.btn);
        brActive  = CreateSolidBrush(theme.active);
        brBlink   = CreateSolidBrush(theme.blink);
        brFill    = CreateSolidBrush(theme.fill);
        brFillExp = CreateSolidBrush(theme.fill_exp);
        brHelp    = CreateSolidBrush(RGB(20, 20, 20));
        pnNull    = CreatePen(PS_NULL, 0, 0);
        pnDivider = CreatePen(PS_SOLID, 1, RGB(50, 50, 55));
    }

    void destroy_brushes() {
        HGDIOBJ objs[] = {brBg, brBar, brBtn, brActive, brBlink, brFill, brFillExp, brHelp, pnNull, pnDivider};
        for (auto h : objs) if (h) DeleteObject(h);
    }

    ~WndState() {
        if (fonts_custom) {
            DeleteObject(hFontBig);
            DeleteObject(hFontLarge);
            DeleteObject(hFontSm);
        }
        destroy_brushes();
        if (mdc) DeleteDC(mdc);
        if (buf_bmp) DeleteObject(buf_bmp);
    }

    WndState(const WndState&) = delete;
    WndState& operator=(const WndState&) = delete;
    WndState() = default;

    // Cached double-buffer DC and bitmap (#52)
    HDC     mdc     = nullptr;
    HBITMAP buf_bmp = nullptr;
    int     buf_w   = 0, buf_h = 0;

    void ensure_buffer(HDC hdc, int w, int h) {
        if (w != buf_w || h != buf_h) {
            if (buf_bmp) DeleteObject(buf_bmp);
            if (mdc) DeleteDC(mdc);
            mdc = CreateCompatibleDC(hdc);
            buf_bmp = CreateCompatibleBitmap(hdc, w, h);
            SelectObject(mdc, buf_bmp);
            buf_w = w; buf_h = h;
        }
    }
};

// Process-wide log file (set before window creation).
static FILE* g_log_file = nullptr;

static LayoutState layout_state(const WndState& s) {
    return {
        .show_clk = s.app.show_clk,
        .show_sw = s.app.show_sw,
        .show_tmr = s.app.show_tmr,
        .timer_count = (int)s.app.timers.size(),
    };
}

static int client_height(const WndState& s) {
    return client_height_for(s.layout, layout_state(s));
}

static void sync_timer(HWND hwnd, WndState& s) {
    bool any_timer_running = false;
    for (auto& ts : s.app.timers)
        if (ts.t.is_running()) { any_timer_running = true; break; }
    int want = (s.app.show_sw && s.app.sw.is_running()) ? POLL_STOPWATCH_MS
             : any_timer_running ? POLL_TIMER_MS
             : POLL_IDLE_MS;
    if (want != s.timer_ms) { s.timer_ms = want; SetTimer(hwnd, 1, want, nullptr); }
}

// ─── Diagnostics ──────────────────────────────────────────────────────────────
static void dbg(const std::wstring& msg) {
    OutputDebugStringW(msg.c_str());
    OutputDebugStringW(L"\n");
    if (g_log_file) { fwprintf(g_log_file, L"%ls\n", msg.c_str()); fflush(g_log_file); }
}

// ─── Config ───────────────────────────────────────────────────────────────────
static std::filesystem::path config_path() {
    // Prefer %APPDATA%/Chronos/ for config
    if (auto* appdata = _wgetenv(L"APPDATA")) {
        auto dir = std::filesystem::path{appdata} / L"Chronos";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (!ec) return dir / L"config.ini";
    }
    // Fallback: next to exe (portable mode)
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    return std::filesystem::path{exe}.parent_path() / "config.ini";
}

static void save_config(HWND hwnd, const WndState& s) {
    auto path = config_path();
    dbg(std::format(L"[chrono] save_config: {}", path.wstring()));
    std::ofstream f(path);
    if (!f) { dbg(L"[chrono] save_config: open failed"); return; }
    Config cfg;
    cfg.show_clk   = s.app.show_clk;
    cfg.show_sw    = s.app.show_sw;
    cfg.show_tmr   = s.app.show_tmr;
    cfg.topmost    = s.app.topmost;
    cfg.num_timers = (int)s.app.timers.size();
    for (int i = 0; i < (int)s.app.timers.size(); ++i)
        cfg.timer_secs[i] = (int)s.app.timers[i].dur.count();
    if (hwnd) {
        RECT wr; GetWindowRect(hwnd, &wr);
        cfg.pos_valid = true;
        cfg.win_x = wr.left;
        cfg.win_y = wr.top;
        cfg.win_w = wr.right - wr.left;
    }
    config_write(cfg, f);
}

static void load_config(HWND hwnd, WndState& s) {
    auto path = config_path();
    dbg(std::format(L"[chrono] load_config: {}", path.wstring()));
    std::ifstream f(path);
    if (!f) { dbg(L"[chrono] no config, using defaults"); return; }
    Config cfg;
    config_read(cfg, f);
    dbg(std::format(L"[chrono] loaded: clk={} sw={} tmr={} top={} pos={} ({},{} w={})",
                    cfg.show_clk, cfg.show_sw, cfg.show_tmr, cfg.topmost,
                    cfg.pos_valid, cfg.win_x, cfg.win_y, cfg.win_w));
    s.app.show_clk = cfg.show_clk;
    s.app.show_sw  = cfg.show_sw;
    s.app.show_tmr = cfg.show_tmr;
    s.app.topmost  = cfg.topmost;
    int n = std::min(cfg.num_timers, Config::MAX_TIMERS);
    s.app.timers.resize(n);
    for (int i = 0; i < n; ++i) {
        s.app.timers[i].dur = seconds{cfg.timer_secs[i]};
        s.app.timers[i].t.reset();
        s.app.timers[i].t.set(s.app.timers[i].dur);
    }
    if (s.app.topmost)
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if (cfg.pos_valid) {
        RECT cur; GetWindowRect(hwnd, &cur);
        // Clamp restored width against the DPI-scaled minimum
        DWORD ws = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
        RECT adj{0, 0, s.layout.bar_min_client_w(), 0};
        AdjustWindowRectEx(&adj, ws, FALSE, 0);
        int min_w = adj.right - adj.left;
        int w = cfg.win_w < min_w ? min_w : cfg.win_w;
        // Validate position against current monitor geometry (#57)
        RECT test{cfg.win_x, cfg.win_y, cfg.win_x + w, cfg.win_y + 1};
        HMONITOR hmon = MonitorFromRect(&test, MONITOR_DEFAULTTONULL);
        if (hmon) {
            SetWindowPos(hwnd, nullptr, cfg.win_x, cfg.win_y,
                         w, cur.bottom - cur.top, SWP_NOZORDER);
        } else {
            // Position is off-screen — apply saved width at default position
            SetWindowPos(hwnd, nullptr, 0, 0,
                         w, cur.bottom - cur.top, SWP_NOZORDER | SWP_NOMOVE);
        }
    }
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
static HFONT make_font(int pt, bool bold, const Layout& layout) {
    int h = -MulDiv(pt, layout.dpi, 72);
    return CreateFontW(h, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                       FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

static void recreate_fonts(WndState& s) {
    if (s.fonts_custom) {
        DeleteObject(s.hFontBig); DeleteObject(s.hFontLarge); DeleteObject(s.hFontSm);
    }
    s.hFontBig   = make_font(26, true, s.layout);
    s.hFontLarge = make_font(34, true, s.layout);
    s.hFontSm    = make_font(11, false, s.layout);
    if (!s.hFontBig || !s.hFontLarge || !s.hFontSm) {
        if (s.hFontBig)   { DeleteObject(s.hFontBig);   s.hFontBig   = nullptr; }
        if (s.hFontLarge) { DeleteObject(s.hFontLarge); s.hFontLarge = nullptr; }
        if (s.hFontSm)    { DeleteObject(s.hFontSm);   s.hFontSm    = nullptr; }
        s.hFontBig   = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        s.hFontLarge = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        s.hFontSm    = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        s.fonts_custom = false;
    } else {
        s.fonts_custom = true;
    }
}

static int nonclient_height(HWND hwnd) {
    RECT wr, cr;
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);
    return (wr.bottom - wr.top) - cr.bottom;
}

// Draw a rounded button, record rect for hit testing, return rect.
static RECT btn(HDC hdc, RECT r, bool active, const wchar_t* label, int id,
                WndState& s, std::optional<COLORREF> override_col = std::nullopt) {
    auto& layout = s.layout;
    bool blinking = id && s.app.blink_act == id &&
                    (sc::now() - s.app.blink_t) < BLINK_DUR;
    // Use cached brushes for standard colors, create only for overrides
    HBRUSH brush;
    GdiObj dyn_br{nullptr};
    if (blinking) {
        brush = s.brBlink;
    } else if (override_col.has_value()) {
        dyn_br.h = CreateSolidBrush(*override_col);
        brush = (HBRUSH)dyn_br.h;
    } else {
        brush = active ? s.brActive : s.brBtn;
    }
    auto*  obr = (HBRUSH)SelectObject(hdc, brush);
    auto*  opn = (HPEN)  SelectObject(hdc, s.pnNull);
    int rr = layout.dpi_scale(6);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rr, rr);
    SelectObject(hdc, obr); SelectObject(hdc, opn);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme.text);
    DrawTextW(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (id) s.btns.push_back({r, id});
    return r;
}

static void divider(HDC hdc, int y, int cw, const WndState& s) {
    auto* op = (HPEN)SelectObject(hdc, s.pnDivider);
    MoveToEx(hdc, 0, y, nullptr);
    LineTo(hdc, cw, y);
    SelectObject(hdc, op);
}

static void resize_window(HWND hwnd, const WndState& s) {
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int cur_w = wr.right - wr.left;
    SetWindowPos(hwnd, nullptr, 0, 0,
                 cur_w, client_height(s) + nonclient_height(hwnd),
                 SWP_NOMOVE | SWP_NOZORDER);
}

// ─── Paint sub-functions ──────────────────────────────────────────────────────

static int paint_bar(HDC hdc, int cw, int y, WndState& s) {
    auto& layout = s.layout;
    RECT bar{0, 0, cw, layout.bar_h};
    FillRect(hdc, &bar, s.brBar);

    SelectObject(hdc, s.hFontSm);
    int by = (layout.bar_h - layout.btn_h) / 2;
    int bx = (cw - (layout.w_pin + layout.w_clk + layout.w_sw + layout.w_tmr + 3*layout.bar_gap)) / 2;
    btn(hdc, {bx, by, bx+layout.w_pin, by+layout.btn_h}, s.app.topmost,  L"Pin",       A_TOPMOST, s);  bx += layout.w_pin+layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_clk, by+layout.btn_h}, s.app.show_clk, L"Clock",     A_SHOW_CLK, s); bx += layout.w_clk+layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_sw,  by+layout.btn_h}, s.app.show_sw,  L"Stopwatch", A_SHOW_SW, s);  bx += layout.w_sw +layout.bar_gap;
    btn(hdc, {bx, by, bx+layout.w_tmr, by+layout.btn_h}, s.app.show_tmr, L"Timers",    A_SHOW_TMR, s);
    return layout.bar_h;
}

static int paint_clock(HDC hdc, int cw, int y, WndState& s) {
    auto& layout = s.layout;
    divider(hdc, y, cw, s);
    SYSTEMTIME st;
    GetLocalTime(&st);
    auto buf = std::format(L"{:02}:{:02}:{:02}", st.wHour, st.wMinute, st.wSecond);
    SelectObject(hdc, s.hFontBig);
    SetTextColor(hdc, theme.text);
    RECT tr{0, y, cw, y + layout.clk_h};
    DrawTextW(hdc, buf.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return y + layout.clk_h;
}

static int paint_stopwatch(HDC hdc, int cw, int y, WndState& s, sc::time_point now) {
    auto& layout = s.layout;
    divider(hdc, y, cw, s);
    auto elap = s.app.sw.elapsed(now);
    std::wstring etime = format_stopwatch_display(elap);

    SelectObject(hdc, s.hFontBig);
    SetTextColor(hdc, theme.text);
    RECT tr{0, y + layout.dpi_scale(4), cw, y + layout.dpi_scale(44)};
    DrawTextW(hdc, etime.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, s.hFontSm);
    bool running = s.app.sw.is_running();
    int  bw = layout.dpi_scale(76), gap = layout.dpi_scale(6), bh = layout.dpi_scale(28);
    int  bx0 = (cw - 3*bw - 2*gap) / 2;
    int  by0 = y + layout.dpi_scale(46);
    btn(hdc, {bx0,            by0, bx0+bw,          by0+bh}, running, running ? L"Stop" : L"Start", A_SW_START, s);
    btn(hdc, {bx0+bw+gap,     by0, bx0+2*bw+gap,    by0+bh}, false,   L"Lap",                       A_SW_LAP, s);
    btn(hdc, {bx0+2*(bw+gap), by0, bx0+3*bw+2*gap,  by0+bh}, false,   L"Reset",                     A_SW_RESET, s);

    bool has_file = !s.app.sw_lap_file.empty();
    int  gbw = layout.dpi_scale(100), gbh = layout.dpi_scale(18);
    auto lap_label = s.app.lap_write_failed ? L"Get Laps (!)" : L"Get Laps";
    auto lap_col = s.app.lap_write_failed ? theme.expire
                 : has_file ? theme.btn : theme.dim;
    btn(hdc, {(cw-gbw)/2, by0+bh+layout.dpi_scale(4), (cw+gbw)/2, by0+bh+layout.dpi_scale(4)+gbh},
        false, lap_label, has_file ? A_SW_GET : 0, s, lap_col);
    return y + layout.sw_h;
}

static int paint_timers(HDC hdc, int cw, int y, WndState& s, sc::time_point now) {
    auto& layout = s.layout;
    int abw = layout.dpi_scale(34), abh = layout.dpi_scale(16), gap = layout.dpi_scale(6), bh = layout.dpi_scale(28);
    int up_off = layout.dpi_scale(4);
    int td_off = up_off + abh + layout.dpi_scale(2);
    int dn_off = td_off + layout.dpi_scale(40) + layout.dpi_scale(2);
    int by_off = dn_off + abh + gap;

    for (int i = 0; i < (int)s.app.timers.size(); ++i) {
        divider(hdc, y, cw, s);
        auto& ts      = s.app.timers[i];
        bool  running = ts.t.is_running();
        bool  touched = ts.t.touched();
        bool  expired = touched && ts.t.expired(now);

        if (touched) {
            HBRUSH fillbr = expired ? s.brFillExp : s.brFill;
            int fw = cw;
            if (!expired) {
                auto total = duration_cast<microseconds>(ts.dur).count();
                auto rem   = duration_cast<microseconds>(ts.t.remaining(now)).count();
                fw = total > 0 ? (int)(cw * (double)(total - rem) / total) : 0;
            }
            RECT fr{0, y, fw, y + layout.tmr_h};
            FillRect(hdc, &fr, fillbr);
        }

        // Three-column layout for H:MM:SS input
        int col_gap = layout.dpi_scale(34);
        int hh_cx = cw/2 - col_gap, mm_cx = cw/2, ss_cx = cw/2 + col_gap;

        SelectObject(hdc, s.hFontSm);
        if (!touched) {
            btn(hdc, {hh_cx-abw/2, y+up_off, hh_cx+abw/2, y+up_off+abh}, false, L"▲", tmr_act(i, A_TMR_HUP), s);
            btn(hdc, {mm_cx-abw/2, y+up_off, mm_cx+abw/2, y+up_off+abh}, false, L"▲", tmr_act(i, A_TMR_MUP), s);
            btn(hdc, {ss_cx-abw/2, y+up_off, ss_cx+abw/2, y+up_off+abh}, false, L"▲", tmr_act(i, A_TMR_SUP), s);
        }

        std::wstring tstr = touched ? format_timer_display(ts.t.remaining(now))
                                    : format_timer_edit(duration_cast<Timer::dur>(ts.dur));
        COLORREF tcol = expired ? theme.expire
            : (touched && ts.t.remaining(now) < 10s) ? theme.warn
            : theme.text;
        if (touched) {
            SelectObject(hdc, s.hFontSm);
            SetTextColor(hdc, theme.dim);
            std::wstring sstr = format_timer_edit(duration_cast<Timer::dur>(ts.dur));
            RECT sr{0, y + up_off, cw, y + up_off + layout.dpi_scale(20)};
            DrawTextW(hdc, sstr.c_str(), -1, &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, s.hFontLarge);
            SetTextColor(hdc, tcol);
            RECT tr2{0, y + up_off + layout.dpi_scale(20), cw, y + dn_off + abh};
            DrawTextW(hdc, tstr.c_str(), -1, &tr2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            SelectObject(hdc, s.hFontBig);
            SetTextColor(hdc, tcol);
            RECT tr2{0, y + td_off, cw, y + td_off + layout.dpi_scale(40)};
            DrawTextW(hdc, tstr.c_str(), -1, &tr2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        SelectObject(hdc, s.hFontSm);
        SetTextColor(hdc, theme.text);
        if (!touched) {
            btn(hdc, {hh_cx-abw/2, y+dn_off, hh_cx+abw/2, y+dn_off+abh}, false, L"▼", tmr_act(i, A_TMR_HDN), s);
            btn(hdc, {mm_cx-abw/2, y+dn_off, mm_cx+abw/2, y+dn_off+abh}, false, L"▼", tmr_act(i, A_TMR_MDN), s);
            btn(hdc, {ss_cx-abw/2, y+dn_off, ss_cx+abw/2, y+dn_off+abh}, false, L"▼", tmr_act(i, A_TMR_SDN), s);
        }

        int cw2 = layout.dpi_scale(86);
        int cx0 = (cw - 2*cw2 - gap) / 2;
        btn(hdc, {cx0,         y+by_off, cx0+cw2,       y+by_off+bh}, running,
            running ? L"Pause" : L"Start", tmr_act(i, A_TMR_START), s);
        btn(hdc, {cx0+cw2+gap, y+by_off, cx0+2*cw2+gap, y+by_off+bh}, false, L"Reset",
            tmr_act(i, A_TMR_RST), s);

        int pm_sz = layout.dpi_scale(22), pm_margin = layout.dpi_scale(6);
        int pm_top = y + layout.tmr_h - pm_sz - layout.dpi_scale(4);
        if ((int)s.app.timers.size() < Config::MAX_TIMERS)
            btn(hdc, {cw-pm_margin-pm_sz, pm_top, cw-pm_margin, pm_top+pm_sz},
                false, L"+", tmr_act(i, A_TMR_ADD), s);
        if ((int)s.app.timers.size() > 1)
            btn(hdc, {pm_margin, pm_top, pm_margin+pm_sz, pm_top+pm_sz},
                false, L"−", tmr_act(i, A_TMR_DEL), s);

        y += layout.tmr_h;
    }
    return y;
}

static void paint_help(HDC hdc, int cw, int y_bottom, WndState& s) {
    auto& layout = s.layout;
    RECT cr{0, layout.bar_h, cw, y_bottom > layout.bar_h ? y_bottom : layout.bar_h + layout.dpi_scale(200)};
    FillRect(hdc, &cr, s.brHelp);

    SelectObject(hdc, s.hFontSm);
    SetTextColor(hdc, theme.text);

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
        auto line = std::format(L"  {}  —  {}", key, desc);
        RECT lr{pad, sy, cw - pad, sy + line_h};
        DrawTextW(hdc, line.c_str(), -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        sy += line_h;
    }
}

// ─── Paint dispatcher ─────────────────────────────────────────────────────────
static void paint_all(HDC hdc, int cw, WndState& s) {
    s.btns.clear();
    SetBkMode(hdc, TRANSPARENT);

    RECT all{0, 0, cw, 9999};
    FillRect(hdc, &all, s.brBg);

    auto now = sc::now();
    int y = paint_bar(hdc, cw, 0, s);
    if (s.app.show_clk) y = paint_clock(hdc, cw, y, s);
    if (s.app.show_sw)  y = paint_stopwatch(hdc, cw, y, s, now);
    if (s.app.show_tmr) y = paint_timers(hdc, cw, y, s, now);
    if (s.app.show_help) paint_help(hdc, cw, y, s);
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

// Result flags from action dispatch — separates "what changed" from "what to do about it"
struct HandleResult {
    bool save_config = false;
    bool resize      = false;
    bool set_topmost = false;
    bool open_file   = false;
};

// Pure domain logic: mutates App state, returns side-effect flags
static HandleResult dispatch_action(App& app, int act, sc::time_point now,
                                    const std::filesystem::path& config_dir) {
    HandleResult r;

    switch (act) {
    case A_TOPMOST:
        app.topmost = !app.topmost;
        r.set_topmost = true;
        r.save_config = true;
        break;
    case A_SHOW_CLK: app.show_clk = !app.show_clk; r.resize = true; r.save_config = true; break;
    case A_SHOW_SW:  app.show_sw  = !app.show_sw;  r.resize = true; r.save_config = true; break;
    case A_SHOW_TMR: app.show_tmr = !app.show_tmr; r.resize = true; r.save_config = true; break;
    case A_SW_START:
        if (!app.sw.is_running()) {
            if (app.sw_lap_file.empty()) {
                SYSTEMTIME st; GetLocalTime(&st);
                auto lap_name = std::format(L"stopwatch-{:04}{:02}{:02}-{:02}{:02}{:02}-{:03}.txt",
                                            st.wYear, st.wMonth, st.wDay,
                                            st.wHour, st.wMinute, st.wSecond,
                                            st.wMilliseconds);
                app.sw_lap_file = (config_dir / lap_name).wstring();
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
                std::wofstream f(app.sw_lap_file, std::ios::app);
                if (f) {
                    const auto& laps = app.sw.laps();
                    auto n = laps.size();
                    sc::duration cum{};
                    for (auto& l : laps) cum += l;
                    f << format_lap_row(n, laps.back(), cum) << L'\n';
                    app.lap_write_failed = false;
                } else {
                    app.lap_write_failed = true;
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
            r.open_file = true;
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
                ts.t.reset(); ts.t.set(ts.dur); ts.notified = false;
            } else if (off == A_TMR_ADD) {
                if ((int)app.timers.size() < Config::MAX_TIMERS) {
                    TimerSlot ns; ns.t.set(ns.dur);
                    app.timers.insert(app.timers.begin() + idx + 1, ns);
                    r.resize = true; r.save_config = true;
                }
            } else if (off == A_TMR_DEL) {
                if ((int)app.timers.size() > 1) {
                    app.timers.erase(app.timers.begin() + idx);
                    r.resize = true; r.save_config = true;
                }
            } else if (!ts.t.touched()) {
                auto total = ts.dur.count();
                int h = (int)(total / 3600);
                int m = (int)((total / 60) % 60);
                int sec = (int)(total % 60);
                bool changed = true;
                if      (off == A_TMR_HUP) h = (h >= 24) ? 0 : h + 1;
                else if (off == A_TMR_HDN) h = (h <= 0)  ? 24 : h - 1;
                else if (off == A_TMR_MUP) m = (m >= 59) ? 0 : m + 1;
                else if (off == A_TMR_MDN) m = (m <= 0)  ? 59 : m - 1;
                else if (off == A_TMR_SUP) sec = (sec >= 59) ? 0 : sec + 1;
                else if (off == A_TMR_SDN) sec = (sec <= 0)  ? 59 : sec - 1;
                else changed = false;
                if (changed) {
                    ts.dur = seconds{h * 3600 + m * 60 + sec};
                    ts.t.reset(); ts.t.set(ts.dur);
                    r.save_config = true;
                }
            }
        }
    }
    return r;
}

// Thin wrapper: dispatches action, applies Win32 side effects
static void handle(HWND hwnd, int act, WndState& s) {
    auto now = sc::now();
    if (wants_blink(act)) { s.app.blink_act = act; s.app.blink_t = now; }

    auto r = dispatch_action(s.app, act, now, config_path().parent_path());

    if (r.set_topmost)
        SetWindowPos(hwnd, s.app.topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if (r.resize)      resize_window(hwnd, s);
    if (r.save_config) save_config(hwnd, s);
    if (r.open_file)
        ShellExecuteW(nullptr, L"open", s.app.sw_lap_file.c_str(),
                      nullptr, nullptr, SW_SHOW);
    InvalidateRect(hwnd, nullptr, FALSE);
    sync_timer(hwnd, s);
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
    auto* s = (WndState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        auto state = std::make_unique<WndState>();
        s = state.get();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)s);
        // Initialize layout from WinMain's DPI calculation, then refine per-window
        auto* cs = (CREATESTRUCTW*)lp;
        if (cs->lpCreateParams)
            s->layout = *(Layout*)cs->lpCreateParams;
        if (pGetDpiForWindow) {
            UINT wdpi = pGetDpiForWindow(hwnd);
            if (wdpi != 0) s->layout.update_for_dpi((int)wdpi);
        }
        recreate_fonts(*s);
        s->create_brushes();
        SetTimer(hwnd, 1, POLL_TIMER_MS, nullptr);
        BOOL dark = system_prefers_dark() ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR,
                              &dark, sizeof(dark));
        load_config(hwnd, *s);
        resize_window(hwnd, *s);
        state.release(); // transfer ownership to HWND userdata
        return 0;
    }
    default:
        break;
    }

    if (!s) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_TIMER: {
        auto now = sc::now();
        for (auto& ts : s->app.timers) {
            if (ts.t.touched() && ts.t.expired(now) && !ts.notified) {
                ts.notified = true;
                MessageBeep(MB_ICONASTERISK);
                FLASHWINFO fi{};
                fi.cbSize    = sizeof(fi);
                fi.hwnd      = hwnd;
                fi.dwFlags   = FLASHW_ALL | FLASHW_TIMERNOFG;
                fi.uCount    = 3;
                fi.dwTimeout = 0;
                FlashWindowEx(&fi);
            }
        }
        // Update window title: running timer > running stopwatch > current time
        {
            std::wstring title;
            for (auto& ts : s->app.timers) {
                if (ts.t.is_running()) {
                    title = format_timer_display(ts.t.remaining(now));
                    if (ts.t.expired(now)) title = L"EXPIRED " + title;
                    break;
                }
            }
            if (title.empty() && s->app.sw.is_running()) {
                title = format_stopwatch_display(s->app.sw.elapsed(now));
            }
            if (title.empty()) {
                SYSTEMTIME st; GetLocalTime(&st);
                title = std::format(L"{:02}:{:02}:{:02}", st.wHour, st.wMinute, st.wSecond);
            }
            title += L" — Chronos";
            SetWindowTextW(hwnd, title.c_str());
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        sync_timer(hwnd, *s);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT cr; GetClientRect(hwnd, &cr);
        s->ensure_buffer(hdc, cr.right, cr.bottom);
        paint_all(s->mdc, cr.right, *s);
        BitBlt(hdc, 0, 0, cr.right, cr.bottom, s->mdc, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        for (auto& [r, id] : s->btns)
            if (PtInRect(&r, pt)) { handle(hwnd, id, *s); break; }
        return 0;
    }
    case WM_KEYDOWN: {
        switch (wp) {
        case VK_SPACE:
            if (s->app.show_sw)
                handle(hwnd, A_SW_START, *s);
            else if (s->app.show_tmr && !s->app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_START), *s);
            break;
        case 'L':
            if (s->app.show_sw) handle(hwnd, A_SW_LAP, *s);
            break;
        case 'R':
            if (s->app.show_sw)
                handle(hwnd, A_SW_RESET, *s);
            else if (s->app.show_tmr && !s->app.timers.empty())
                handle(hwnd, tmr_act(0, A_TMR_RST), *s);
            break;
        case 'T':
            handle(hwnd, A_TOPMOST, *s);
            break;
        case '1': case '2': case '3': {
            int idx = (int)(wp - '1');
            if (s->app.show_tmr && idx < (int)s->app.timers.size())
                handle(hwnd, tmr_act(idx, A_TMR_START), *s);
            break;
        }
        case 'H':
            s->app.show_help = !s->app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
        return 0;
    }
    case WM_CHAR:
        if (wp == '?') {
            s->app.show_help = !s->app.show_help;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_SIZING: {
        int want_h = client_height(*s) + nonclient_height(hwnd);
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
        RECT adj{0, 0, s->layout.bar_min_client_w(), 0};
        AdjustWindowRectEx(&adj, ws, FALSE, 0);
        m->ptMinTrackSize.x = adj.right - adj.left;
        RECT adj_h{0, 0, 0, client_height(*s)};
        AdjustWindowRectEx(&adj_h, ws, FALSE, 0);
        m->ptMinTrackSize.y = adj_h.bottom - adj_h.top;
        return 0;
    }
    case WM_DPICHANGED: {
        s->layout.update_for_dpi(HIWORD(wp));
        recreate_fonts(*s);
        auto* suggested = (RECT*)lp;
        SetWindowPos(hwnd, nullptr,
                     suggested->left, suggested->top,
                     suggested->right - suggested->left,
                     suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        resize_window(hwnd, *s);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_SETTINGCHANGE:
        if (lp && wcscmp((LPCWSTR)lp, L"ImmersiveColorSet") == 0) {
            BOOL dark = system_prefers_dark() ? TRUE : FALSE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR,
                                  &dark, sizeof(dark));
        }
        return 0;
    case WM_EXITSIZEMOVE:
        save_config(hwnd, *s);
        return 0;
    case WM_DESTROY: {
        save_config(hwnd, *s);
        KillTimer(hwnd, 1);
        auto owned = std::unique_ptr<WndState>(s);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return 0;
    }
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

    // Load DPI APIs dynamically and set per-monitor DPI awareness v2
    load_dpi_apis();
    if (pSetProcessDpiAwarenessContext)
        pSetProcessDpiAwarenessContext(DPI_CTX_PER_MONITOR_V2);

    // Get initial DPI from primary monitor before window exists
    Layout init_layout;
    HDC dc = GetDC(nullptr);
    init_layout.update_for_dpi(GetDeviceCaps(dc, LOGPIXELSY));
    ReleaseDC(nullptr, dc);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ChronoApp";
    RegisterClassExW(&wc);

    constexpr DWORD ws = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
    int init_h = init_layout.bar_h + init_layout.clk_h + init_layout.sw_h + init_layout.tmr_h;
    RECT wr{0, 0, init_layout.bar_min_client_w(), init_h};
    AdjustWindowRect(&wr, ws, FALSE);

    HWND hwnd = CreateWindowExW(
        0, L"ChronoApp", L"Chronos", ws,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInst, &init_layout);

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
