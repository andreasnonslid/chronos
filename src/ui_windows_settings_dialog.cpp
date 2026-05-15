#include "ui_windows_settings_dialog.hpp"
// Settings dialog — full implementation. Public entry point: ui::show_settings_dialog().
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <format>
#include <vector>
#include "app.hpp"
#include "dialog_style.hpp"
#include "painting_analog.hpp"

namespace ui {

namespace settings_dlg {

// ─── IDs & constants ──────────────────────────────────────────────────────────

constexpr int IDC_POMO_WORK    = 301;
constexpr int IDC_POMO_SHORT   = 302;
constexpr int IDC_POMO_LONG    = 303;
constexpr int IDC_LBL_WORK     = 304;
constexpr int IDC_LBL_SHORT    = 305;
constexpr int IDC_LBL_LONG     = 306;
constexpr int IDC_MIN_WORK     = 307;
constexpr int IDC_MIN_SHORT    = 308;
constexpr int IDC_MIN_LONG     = 309;
constexpr int IDC_POMO_CADENCE = 310;
constexpr int IDC_LBL_CADENCE  = 311;
constexpr int IDC_MIN_CADENCE  = 312;
constexpr int IDC_SET_OK       = 313;
constexpr int IDC_SET_CANCEL   = 314;
constexpr int IDC_PRESET_ED0   = 320;
constexpr int IDC_PRESET_ED1   = 321;
constexpr int IDC_PRESET_ED2   = 322;
constexpr int IDC_PRESET_ED3   = 323;
constexpr int IDC_PRESET_ED4   = 324;
constexpr int IDC_PRESET_MIN0  = 325;
constexpr int IDC_PRESET_MIN1  = 326;
constexpr int IDC_PRESET_MIN2  = 327;
constexpr int IDC_PRESET_MIN3  = 328;
constexpr int IDC_PRESET_MIN4  = 329;
constexpr int IDC_CLOCK_COMBO  = 330;

constexpr int IDC_THEME_AUTO  = 340;
constexpr int IDC_THEME_LIGHT = 341;
constexpr int IDC_THEME_DARK  = 342;
constexpr int IDC_SOUND       = 343;
constexpr int IDC_AUTO_START  = 344;

constexpr WORD CLS_BUTTON   = 0x0080;
constexpr WORD CLS_EDIT     = 0x0081;
constexpr WORD CLS_STATIC   = 0x0082;
constexpr WORD CLS_COMBOBOX = 0x0085;

constexpr short DLG_W = 290, DLG_H = 185;
constexpr short SIDEBAR_W = 62;
constexpr short TITLE_H = 18;

constexpr int TAB_APPEARANCE = 0;
constexpr int TAB_CLOCK      = 1;
constexpr int TAB_POMODORO   = 2;
constexpr int TAB_TIMERS     = 3;
constexpr int TAB_COUNT      = 4;

// ─── Dialog template builder ──────────────────────────────────────────────────

constexpr WORD CTRL_COUNT = 30;

static std::vector<WORD> build_template() {
    DlgBuf b;
    b.push_dword(WS_POPUP | WS_THICKFRAME | WS_VSCROLL);
    b.push_dword(0);
    b.push_word(CTRL_COUNT);
    b.push_word(0); b.push_word(0);
    b.push_word(DLG_W); b.push_word(DLG_H);
    b.push_wstr_empty();
    b.push_wstr_empty();
    b.push_wstr_empty();

    short lbl_x = 72, lbl_w = 50;
    short ed_x = 126, ed_w = 28;
    short min_x = 158, min_w = 18;
    short row_h = 10;
    short y0 = 40, sp = 18;

    dlg_add_item(b, SS_RIGHT | SS_CENTERIMAGE,
             lbl_x, y0, lbl_w, row_h, IDC_LBL_WORK, CLS_STATIC, L"Work");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
             ed_x, y0, ed_w, row_h, IDC_POMO_WORK, CLS_EDIT, L"");
    dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
             min_x, y0, min_w, row_h, IDC_MIN_WORK, CLS_STATIC, L"min");

    short r1 = y0 + sp;
    dlg_add_item(b, SS_RIGHT | SS_CENTERIMAGE,
             lbl_x, r1, lbl_w, row_h, IDC_LBL_SHORT, CLS_STATIC, L"Short break");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
             ed_x, r1, ed_w, row_h, IDC_POMO_SHORT, CLS_EDIT, L"");
    dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
             min_x, r1, min_w, row_h, IDC_MIN_SHORT, CLS_STATIC, L"min");

    short r2 = y0 + 2 * sp;
    dlg_add_item(b, SS_RIGHT | SS_CENTERIMAGE,
             lbl_x, r2, lbl_w, row_h, IDC_LBL_LONG, CLS_STATIC, L"Long break");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
             ed_x, r2, ed_w, row_h, IDC_POMO_LONG, CLS_EDIT, L"");
    dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
             min_x, r2, min_w, row_h, IDC_MIN_LONG, CLS_STATIC, L"min");

    short r3 = y0 + 3 * sp;
    dlg_add_item(b, SS_RIGHT | SS_CENTERIMAGE,
             lbl_x, r3, lbl_w, row_h, IDC_LBL_CADENCE, CLS_STATIC, L"Long every");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
             ed_x, r3, ed_w, row_h, IDC_POMO_CADENCE, CLS_EDIT, L"");
    dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
             min_x, r3, min_w + 20, row_h, IDC_MIN_CADENCE, CLS_STATIC, L"sessions");

    short pr_ed_x = 96, pr_ed_w = 28;
    short pr_min_x = 128, pr_min_w = 18;
    short pr_y0 = 40, pr_sp = 16;
    int pr_edit_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                         IDC_PRESET_ED3, IDC_PRESET_ED4};
    int pr_min_ids[]  = {IDC_PRESET_MIN0, IDC_PRESET_MIN1, IDC_PRESET_MIN2,
                         IDC_PRESET_MIN3, IDC_PRESET_MIN4};
    for (int i = 0; i < 5; ++i) {
        short py = pr_y0 + (short)(i * pr_sp);
        dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
                 pr_ed_x, py, pr_ed_w, row_h, (WORD)pr_edit_ids[i], CLS_EDIT, L"");
        dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
                 pr_min_x, py, pr_min_w, row_h, (WORD)pr_min_ids[i], CLS_STATIC, L"min");
    }

    // Clock format dropdown — height includes dropdown list space for 5 items
    dlg_add_item(b, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
             68, 40, 212, 80, IDC_CLOCK_COMBO, CLS_COMBOBOX, L"");

    // Theme radio buttons (Appearance tab)
    dlg_add_item(b, WS_GROUP | WS_TABSTOP | BS_OWNERDRAW,
             70, 42, 54, 13, IDC_THEME_AUTO, CLS_BUTTON, L"Auto");
    dlg_add_item(b, BS_OWNERDRAW,
             70, 57, 54, 13, IDC_THEME_LIGHT, CLS_BUTTON, L"Light");
    dlg_add_item(b, BS_OWNERDRAW,
             70, 72, 54, 13, IDC_THEME_DARK, CLS_BUTTON, L"Dark");

    // Sound checkbox (Appearance tab)
    dlg_add_item(b, WS_GROUP | WS_TABSTOP | BS_OWNERDRAW,
             70, 97, 100, 13, IDC_SOUND, CLS_BUTTON, L"Sound");

    // Auto-start checkbox (Pomodoro tab)
    dlg_add_item(b, WS_GROUP | WS_TABSTOP | BS_OWNERDRAW,
             70, 115, 100, 13, IDC_AUTO_START, CLS_BUTTON, L"Auto-start");

    short btn_w = 44, btn_h = 16, btn_gap = 8;
    short content_cx = DLG_W - SIDEBAR_W - 4;
    short total_bw = 2 * btn_w + btn_gap;
    short btn_x0 = SIDEBAR_W + 2 + (content_cx - total_bw) / 2;
    short btn_y = DLG_H - 24;
    dlg_add_item(b, BS_OWNERDRAW | WS_TABSTOP,
             btn_x0, btn_y, btn_w, btn_h, IDC_SET_OK, CLS_BUTTON, L"Apply");
    dlg_add_item(b, BS_OWNERDRAW | WS_TABSTOP,
             btn_x0 + btn_w + btn_gap, btn_y, btn_w, btn_h, IDC_SET_CANCEL, CLS_BUTTON, L"Cancel");

    return b.data;
}

// ─── State ────────────────────────────────────────────────────────────────────

static RECT map_dlu(HWND dlg, short x, short y, short cx, short cy) {
    RECT r = {x, y, x + cx, y + cy};
    MapDialogRect(dlg, &r);
    return r;
}

constexpr int ANALOG_COLOR_COUNT = 9;
constexpr int ANALOG_VALUE_COUNT = 15;

struct HitRects {
    RECT sidebar[TAB_COUNT];
    RECT preset_row[5];
    RECT analog_preview;
    RECT analog_min_ticks;
    RECT analog_labels[3];
    RECT analog_colors[ANALOG_COLOR_COUNT];
    RECT analog_values[ANALOG_VALUE_COUNT];
};

static HitRects compute_rects(HWND dlg) {
    HitRects h{};
    h.sidebar[0] = map_dlu(dlg, 3, 24, 56, 14);
    h.sidebar[1] = map_dlu(dlg, 3, 40, 56, 14);
    h.sidebar[2] = map_dlu(dlg, 3, 56, 56, 14);
    h.sidebar[3] = map_dlu(dlg, 3, 72, 56, 14);

    // Analog settings sit below the format dropdown (which is at y=40, h=12).
    // "Live preview" label at y=64 (h=8), preview fixed (sticky) at y=76; right column starts at x=158.
    h.analog_preview = map_dlu(dlg, 68, 76, 80, 80);
    h.analog_min_ticks = map_dlu(dlg, 158,  64, 122, 12);
    h.analog_labels[0] = map_dlu(dlg, 158,  92,  40, 12);
    h.analog_labels[1] = map_dlu(dlg, 202,  92,  40, 12);
    h.analog_labels[2] = map_dlu(dlg, 246,  92,  40, 12);
    for (int i = 0; i < ANALOG_COLOR_COUNT; ++i)
        h.analog_colors[i] = map_dlu(dlg, 158, (short)(122 + i * 14), 122, 12);
    for (int i = 0; i < ANALOG_VALUE_COUNT; ++i)
        h.analog_values[i] = map_dlu(dlg, 68, (short)(258 + i * 14), 214, 12);

    for (int i = 0; i < 5; ++i)
        h.preset_row[i] = map_dlu(dlg, 96, (short)(40 + i * 16), 50, 12);
    return h;
}

struct Params {
    int active_tab = TAB_APPEARANCE;
    ThemeMode theme_mode;
    ClockView clock_view;
    AnalogClockStyle analog_style;
    bool sound_on_expiry;
    int work_min, short_min, long_min;
    int cadence;
    bool auto_start;
    int preset_min[5]{};
    DlgStyle style;
    HitRects rects{};
    int scroll_y = 0;       // current scroll offset in pixels
    RECT clock_combo_rc{};  // base pixel rect of the combobox (unscrolled)
    DialogBrushes brushes;
};

// ─── Scroll & visibility ──────────────────────────────────────────────────────

// Returns the pixel y where the scrollable content area ends (top of button row).
static int content_area_bottom(HWND dlg) {
    HWND ok_btn = GetDlgItem(dlg, IDC_SET_OK);
    if (ok_btn) {
        RECT r{};
        GetWindowRect(ok_btn, &r);
        MapWindowPoints(HWND_DESKTOP, dlg, reinterpret_cast<POINT*>(&r), 2);
        if (r.top > 0) {
            RECT margin{0, 0, 0, 4};
            MapDialogRect(dlg, &margin);
            return r.top - margin.bottom;
        }
    }
    RECT r{0, 0, 0, DLG_H - 24};
    MapDialogRect(dlg, &r);
    return r.bottom;
}

static void set_appearance_visible(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    for (int id = IDC_THEME_AUTO; id <= IDC_SOUND; ++id) {
        HWND h = GetDlgItem(dlg, id);
        if (h) ShowWindow(h, cmd);
    }
}

static void set_pomo_visible(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    for (int id = IDC_POMO_WORK; id <= IDC_MIN_CADENCE; ++id) {
        HWND h = GetDlgItem(dlg, id);
        if (h) ShowWindow(h, cmd);
    }
    HWND h = GetDlgItem(dlg, IDC_AUTO_START);
    if (h) ShowWindow(h, cmd);
}

static void set_presets_visible(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    int ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2, IDC_PRESET_ED3, IDC_PRESET_ED4,
                 IDC_PRESET_MIN0, IDC_PRESET_MIN1, IDC_PRESET_MIN2, IDC_PRESET_MIN3, IDC_PRESET_MIN4};
    for (int id : ids) {
        HWND h = GetDlgItem(dlg, id);
        if (h) ShowWindow(h, cmd);
    }
}

static void set_clock_combo_visible(HWND dlg, bool show) {
    HWND h = GetDlgItem(dlg, IDC_CLOCK_COMBO);
    if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
}

static void position_scrollable_controls(HWND dlg, const Params& p) {
    if (p.active_tab != TAB_CLOCK) return;

    HWND combo = GetDlgItem(dlg, IDC_CLOCK_COMBO);
    if (!combo || !IsWindowVisible(combo)) return;

    SetWindowPos(combo, nullptr,
                 p.clock_combo_rc.left,
                 p.clock_combo_rc.top - p.scroll_y,
                 0, 0,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

static int rect_bottom_after_scroll(const RECT& r, const Params& p) {
    return r.bottom - p.scroll_y;
}

static int tab_painted_content_bottom(HWND dlg, const Params& p) {
    switch (p.active_tab) {
    case TAB_CLOCK: {
        if (p.clock_view != ClockView::Analog) {
            return map_dlu(dlg, 70, 28, 80, 12).bottom;
        }
        // analog_preview is fixed/sticky and excluded from scroll content
        int bottom = std::max({
            rect_bottom_after_scroll(p.rects.analog_min_ticks, p),
            rect_bottom_after_scroll(p.rects.analog_labels[0], p),
            rect_bottom_after_scroll(p.rects.analog_labels[1], p),
            rect_bottom_after_scroll(p.rects.analog_labels[2], p),
        });
        for (int i = 0; i < ANALOG_COLOR_COUNT; ++i)
            bottom = std::max(bottom, rect_bottom_after_scroll(p.rects.analog_colors[i], p));
        for (int i = 0; i < ANALOG_VALUE_COUNT; ++i)
            bottom = std::max(bottom, rect_bottom_after_scroll(p.rects.analog_values[i], p));
        return bottom;
    }
    case TAB_TIMERS:
        return map_dlu(dlg, 76, 104, 16, 12).bottom;
    default:
        return 0;
    }
}

static bool control_counts_as_scroll_content(int id) {
    return id != IDC_SET_OK && id != IDC_SET_CANCEL;
}

static int visible_child_content_bottom(HWND dlg) {
    int content_end = 0;
    EnumChildWindows(dlg, [](HWND child, LPARAM lp) -> BOOL {
        if (!IsWindowVisible(child)) return TRUE;
        int id = GetDlgCtrlID(child);
        if (!control_counts_as_scroll_content(id)) return TRUE;

        RECT r{};
        GetWindowRect(child, &r);
        MapWindowPoints(HWND_DESKTOP, GetParent(child), reinterpret_cast<POINT*>(&r), 2);
        auto* end = reinterpret_cast<int*>(lp);
        *end = std::max(*end, (int)r.bottom);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&content_end));
    return content_end;
}

// Recompute scrollbar range from the actual visible controls plus painted tab content.
static void update_content_scroll(HWND dlg, Params* p) {
    p->scroll_y = 0;
    position_scrollable_controls(dlg, *p);

    RECT div_top = {0, 0, 0, TITLE_H + 2};
    MapDialogRect(dlg, &div_top);

    int view_h = content_area_bottom(dlg) - div_top.bottom;
    int content_bottom = std::max(visible_child_content_bottom(dlg), tab_painted_content_bottom(dlg, *p));
    int content_h = std::max(0, content_bottom - (int)div_top.bottom);
    int max_scroll = std::max(0, content_h - view_h);

    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = content_h;
    si.nPage  = (UINT)view_h;
    si.nPos   = 0;
    SetScrollInfo(dlg, SB_VERT, &si, TRUE);
    EnableScrollBar(dlg, SB_VERT, max_scroll > 0 ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
}

// Apply scroll delta and reposition the combobox if on the clock tab.
static void apply_scroll(HWND dlg, Params* p, int new_pos) {
    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_POS, 0, 0, 0, 0, 0};
    si.nPos = new_pos;
    SetScrollInfo(dlg, SB_VERT, &si, TRUE);
    GetScrollInfo(dlg, SB_VERT, &si);
    p->scroll_y = si.nPos;

    position_scrollable_controls(dlg, *p);
    InvalidateRect(dlg, nullptr, TRUE);
}

// ─── Paint helpers ────────────────────────────────────────────────────────────

static void paint_option_btn(HDC hdc, const RECT& rc, const wchar_t* text, bool selected,
                              const DlgStyle& s) {
    int rr = s.scale(4);
    HBRUSH br = CreateSolidBrush(selected ? s.theme->active : s.theme->btn);
    HPEN pen = CreatePen(PS_NULL, 0, 0);
    auto* obr = (HBRUSH)SelectObject(hdc, br);
    auto* opn = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, rr, rr);
    SelectObject(hdc, obr);
    SelectObject(hdc, opn);
    DeleteObject(br);
    DeleteObject(pen);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, s.theme->text);
    SelectObject(hdc, s.font);
    RECT r = rc;
    DrawTextW(hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static bool read_field(HWND dlg, int id, int& out) {
    wchar_t buf[8] = {};
    GetDlgItemTextW(dlg, id, buf, 7);
    wchar_t* end = nullptr;
    long v = std::wcstol(buf, &end, 10);
    if (!end || *end != L'\0' || v < 1 || v > 1440) return false;
    out = (int)v;
    return true;
}

// ─── Handlers & DlgProc ───────────────────────────────────────────────────────

static Params* dialog_params(HWND dlg) {
    return reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
}

static constexpr struct { ThemeMode mode; int id; } kThemeMap[] = {
    {ThemeMode::Auto,  IDC_THEME_AUTO},
    {ThemeMode::Light, IDC_THEME_LIGHT},
    {ThemeMode::Dark,  IDC_THEME_DARK},
};

static void tab_appearance_init(HWND dlg, Params& p) {
    for (auto& e : kThemeMap)
        if (p.theme_mode == e.mode) { CheckRadioButton(dlg, IDC_THEME_AUTO, IDC_THEME_DARK, e.id); break; }
    CheckDlgButton(dlg, IDC_SOUND, p.sound_on_expiry ? BST_CHECKED : BST_UNCHECKED);
}

static void tab_pomodoro_init(HWND dlg, Params& p) {
    SetDlgItemTextW(dlg, IDC_POMO_WORK,    std::format(L"{}", p.work_min).c_str());
    SetDlgItemTextW(dlg, IDC_POMO_SHORT,   std::format(L"{}", p.short_min).c_str());
    SetDlgItemTextW(dlg, IDC_POMO_LONG,    std::format(L"{}", p.long_min).c_str());
    SetDlgItemTextW(dlg, IDC_POMO_CADENCE, std::format(L"{}", p.cadence).c_str());
    SendDlgItemMessageW(dlg, IDC_POMO_WORK,    EM_SETLIMITTEXT, 4, 0);
    SendDlgItemMessageW(dlg, IDC_POMO_SHORT,   EM_SETLIMITTEXT, 4, 0);
    SendDlgItemMessageW(dlg, IDC_POMO_LONG,    EM_SETLIMITTEXT, 4, 0);
    SendDlgItemMessageW(dlg, IDC_POMO_CADENCE, EM_SETLIMITTEXT, 2, 0);
    CheckDlgButton(dlg, IDC_AUTO_START, p.auto_start ? BST_CHECKED : BST_UNCHECKED);
}

static void tab_timers_init(HWND dlg, Params& p) {
    int pr_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                    IDC_PRESET_ED3, IDC_PRESET_ED4};
    for (int i = 0; i < 5; ++i) {
        if (p.preset_min[i] > 0)
            SetDlgItemTextW(dlg, pr_ids[i], std::format(L"{}", p.preset_min[i]).c_str());
        SendDlgItemMessageW(dlg, pr_ids[i], EM_SETLIMITTEXT, 4, 0);
    }
}

static void tab_clock_init(HWND dlg, Params& p) {
    HWND combo = GetDlgItem(dlg, IDC_CLOCK_COMBO);
    if (!combo) return;

    const wchar_t* clock_names[CLOCK_VIEW_COUNT] = {
        L"24h + seconds", L"24h", L"12h + seconds", L"12h", L"Analog"
    };
    for (int i = 0; i < CLOCK_VIEW_COUNT; ++i)
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)clock_names[i]);
    SendMessageW(combo, CB_SETCURSEL, (WPARAM)(int)p.clock_view, 0);
}

static void set_active_tab(HWND dlg, Params& p, int tab) {
    set_appearance_visible(dlg, false);
    set_pomo_visible(dlg, false);
    set_presets_visible(dlg, false);
    set_clock_combo_visible(dlg, false);

    p.active_tab = tab;

    set_appearance_visible(dlg, tab == TAB_APPEARANCE);
    set_pomo_visible(dlg, tab == TAB_POMODORO);
    set_presets_visible(dlg, tab == TAB_TIMERS);
    set_clock_combo_visible(dlg, tab == TAB_CLOCK);
    update_content_scroll(dlg, &p);
    InvalidateRect(dlg, nullptr, TRUE);
}

static void set_initial_tab_visibility(HWND dlg, Params& p) {
    set_appearance_visible(dlg, p.active_tab == TAB_APPEARANCE);
    set_pomo_visible(dlg, p.active_tab == TAB_POMODORO);
    set_presets_visible(dlg, p.active_tab == TAB_TIMERS);
    set_clock_combo_visible(dlg, p.active_tab == TAB_CLOCK);
    update_content_scroll(dlg, &p);
}

static int* analog_color_field(AnalogClockStyle& a, int i) {
    switch (i) {
    case 0: return &a.hour_color;
    case 1: return &a.minute_color;
    case 2: return &a.second_color;
    case 3: return &a.background_color;
    case 4: return &a.face_fill_color;
    case 5: return &a.face_outline_color;
    case 6: return &a.tick_color;
    case 7: return &a.hour_label_color;
    case 8: return &a.center_dot_color;
    default: return nullptr;
    }
}

static int* analog_value_field(AnalogClockStyle& a, int i) {
    switch (i) {
    case 0: return &a.hour_len_pct;
    case 1: return &a.minute_len_pct;
    case 2: return &a.second_len_pct;
    case 3: return &a.hour_thickness;
    case 4: return &a.minute_thickness;
    case 5: return &a.second_thickness;
    case 6: return &a.hour_tick_pct;
    case 7: return &a.minute_tick_pct;
    case 8: return &a.center_dot_size;
    case 9: return &a.hour_opacity_pct;
    case 10: return &a.minute_opacity_pct;
    case 11: return &a.second_opacity_pct;
    case 12: return &a.tick_opacity_pct;
    case 13: return &a.face_opacity_pct;
    case 14: return &a.radius_pct;
    default: return nullptr;
    }
}

static void analog_value_range(int i, int& min_v, int& max_v, int& step) {
    struct Range { int min_v, max_v, step; };
    static constexpr Range ranges[ANALOG_VALUE_COUNT] = {
        {0, 100, 5}, {0, 100, 5}, {0, 100, 5},
        {1, 6, 1}, {1, 4, 1}, {1, 3, 1},
        {5, 20, 1}, {2, 10, 1}, {0, 10, 1},
        {10, 100, 10}, {10, 100, 10}, {10, 100, 10},
        {10, 100, 10}, {0, 100, 10}, {50, 100, 5},
    };
    min_v = ranges[i].min_v;
    max_v = ranges[i].max_v;
    step = ranges[i].step;
}

static const wchar_t* analog_color_label(int i) {
    static constexpr const wchar_t* labels[ANALOG_COLOR_COUNT] = {
        L"Hour", L"Minute", L"Second", L"Background", L"Face fill",
        L"Outline", L"Ticks", L"Numbers", L"Center dot"
    };
    return labels[i];
}

static const wchar_t* analog_value_label(int i) {
    static constexpr const wchar_t* labels[ANALOG_VALUE_COUNT] = {
        L"Hour length", L"Minute length", L"Second length",
        L"Hour thickness", L"Minute thickness", L"Second thickness",
        L"Hour tick", L"Minute tick", L"Center dot",
        L"Hour opacity", L"Minute opacity", L"Second opacity",
        L"Tick opacity", L"Face opacity", L"Clock radius"
    };
    return labels[i];
}

static constexpr COLORREF ANALOG_PALETTE_COLORS[] = {
    RGB(220, 70, 70), RGB(236, 145, 42), RGB(240, 200, 60),
    RGB(86, 180, 100), RGB(65, 160, 220), RGB(160, 110, 230),
    RGB(235, 235, 235), RGB(35, 35, 35),
};
static constexpr int ANALOG_CUSTOM_PALETTE_COUNT =
    (int)(sizeof(ANALOG_PALETTE_COLORS) / sizeof(ANALOG_PALETTE_COLORS[0]));
static constexpr int ANALOG_PALETTE_COUNT = ANALOG_CUSTOM_PALETTE_COUNT + 1; // auto + custom colors

static COLORREF analog_palette_color(int palette_i) {
    if (palette_i == 0) return (COLORREF)-1;
    return ANALOG_PALETTE_COLORS[(palette_i - 1) % ANALOG_CUSTOM_PALETTE_COUNT];
}

static int analog_palette_index(int color) {
    if (color < 0) return 0;
    for (int i = 1; i < ANALOG_PALETTE_COUNT; ++i)
        if ((COLORREF)color == analog_palette_color(i)) return i;
    return 0;
}

static void reposition_buttons(HWND dlg) {
    HWND ok_btn = GetDlgItem(dlg, IDC_SET_OK);
    HWND cancel_btn = GetDlgItem(dlg, IDC_SET_CANCEL);
    if (!ok_btn || !cancel_btn) return;

    RECT rc{};
    GetClientRect(dlg, &rc);

    RECT ok_wr{}, cancel_wr{};
    GetWindowRect(ok_btn, &ok_wr);
    GetWindowRect(cancel_btn, &cancel_wr);
    int bw = ok_wr.right - ok_wr.left;
    int bh = ok_wr.bottom - ok_wr.top;
    int cw = cancel_wr.right - cancel_wr.left;

    RECT gaps{0, 0, 8, 4};
    MapDialogRect(dlg, &gaps);
    int gap = gaps.right;
    int bot_margin = gaps.bottom;

    RECT sb{0, 0, SIDEBAR_W, 0};
    MapDialogRect(dlg, &sb);
    int content_left = sb.right;
    int content_w = rc.right - content_left;
    int total_bw = bw + gap + cw;
    int btn_x0 = content_left + (content_w - total_bw) / 2;
    int btn_y = rc.bottom - bh - bot_margin;

    SetWindowPos(ok_btn,     nullptr, btn_x0,            btn_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(cancel_btn, nullptr, btn_x0 + bw + gap, btn_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

static INT_PTR on_resize(HWND dlg, WPARAM wp) {
    if (wp == SIZE_MINIMIZED) return FALSE;
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;
    reposition_buttons(dlg);
    update_content_scroll(dlg, p);
    InvalidateRect(dlg, nullptr, TRUE);
    return TRUE;
}

static INT_PTR on_get_min_max_info(HWND dlg, LPARAM lp) {
    auto* mm = reinterpret_cast<MINMAXINFO*>(lp);
    RECT r{0, 0, DLG_W, DLG_H};
    MapDialogRect(dlg, &r);
    int frame_x = GetSystemMetrics(SM_CXSIZEFRAME);
    int frame_y = GetSystemMetrics(SM_CYSIZEFRAME);
    mm->ptMinTrackSize = {r.right + 2 * frame_x, r.bottom + 2 * frame_y};
    return TRUE;
}

static INT_PTR on_init(HWND dlg, LPARAM lp) {
    auto* p = reinterpret_cast<Params*>(lp);
    SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)p);
    p->style.apply_dark_mode(dlg);
    dialog_center_on_parent(dlg);

    p->rects = compute_rects(dlg);
    p->clock_combo_rc = map_dlu(dlg, 68, 40, 212, 80);
    p->brushes = DialogBrushes::create(*p->style.theme);

    tab_appearance_init(dlg, *p);
    tab_pomodoro_init(dlg, *p);
    tab_timers_init(dlg, *p);
    tab_clock_init(dlg, *p);
    dialog_apply_font_to_children(dlg, p->style.font);
    set_initial_tab_visibility(dlg, *p);
    reposition_buttons(dlg);
    return FALSE;
}

static INT_PTR on_erase_background(HWND dlg, HDC hdc) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    RECT rc{};
    GetClientRect(dlg, &rc);
    auto& s = p->style;

    s.fill_background(hdc, rc);

    RECT div_y = {0, 0, 0, TITLE_H + 2};
    MapDialogRect(dlg, &div_y);
    RECT sb = {0, 0, SIDEBAR_W, DLG_H};
    MapDialogRect(dlg, &sb);
    sb.top = div_y.bottom;
    sb.bottom = rc.bottom;
    HBRUSH bar_br = CreateSolidBrush(s.theme->bar);
    FillRect(hdc, &sb, bar_br);
    DeleteObject(bar_br);

    RECT title_rc = {0, 0, rc.right, div_y.bottom};
    s.draw_label(hdc, title_rc, L"Settings", DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HPEN div_pen = CreatePen(PS_SOLID, 1, s.theme->divider);
    auto* old_pen = (HPEN)SelectObject(hdc, div_pen);

    MoveToEx(hdc, 0, div_y.bottom, nullptr);
    LineTo(hdc, rc.right, div_y.bottom);

    MoveToEx(hdc, sb.right, div_y.bottom, nullptr);
    LineTo(hdc, sb.right, rc.bottom);

    int btn_area_top = content_area_bottom(dlg);
    MoveToEx(hdc, sb.right, btn_area_top, nullptr);
    LineTo(hdc, rc.right, btn_area_top);

    SelectObject(hdc, old_pen);
    DeleteObject(div_pen);

    const wchar_t* tab_names[] = {L"Appearance", L"Clock", L"Pomodoro", L"Timers"};
    for (int i = 0; i < TAB_COUNT; ++i)
        paint_option_btn(hdc, p->rects.sidebar[i], tab_names[i], i == p->active_tab, s);

    int save_dc = SaveDC(hdc);
    IntersectClipRect(hdc, sb.right + 1, div_y.bottom + 1, rc.right, btn_area_top);

    int sdy = -p->scroll_y;

    if (p->active_tab == TAB_APPEARANCE) {
        RECT lbl = map_dlu(dlg, 70, 28, 80, 12);
        lbl.top += sdy; lbl.bottom += sdy;
        s.draw_label(hdc, lbl, L"Theme");

        RECT slbl = map_dlu(dlg, 70, 88, 80, 8);
        slbl.top += sdy; slbl.bottom += sdy;
        s.draw_label(hdc, slbl, L"Notifications");

    } else if (p->active_tab == TAB_CLOCK) {
        RECT lbl = map_dlu(dlg, 70, 28, 80, 12);
        lbl.top += sdy; lbl.bottom += sdy;
        s.draw_label(hdc, lbl, L"Format");

        if (p->clock_view == ClockView::Analog) {
            RECT preview = p->rects.analog_preview;

            // Draw scrollable settings clipped to exclude the sticky preview region
            {
                int sdc2 = SaveDC(hdc);
                ExcludeClipRect(hdc, preview.left, preview.top, preview.right, preview.bottom);

                RECT min_r = p->rects.analog_min_ticks;
                min_r.top += sdy; min_r.bottom += sdy;
                paint_option_btn(hdc, min_r,
                                 p->analog_style.show_minute_ticks ? L"Min ticks: on" : L"Min ticks: off",
                                 p->analog_style.show_minute_ticks, s);

                RECT hrlbl = map_dlu(dlg, 158, 82, 80, 8);
                hrlbl.top += sdy; hrlbl.bottom += sdy;
                s.draw_label(hdc, hrlbl, L"Hour labels");

                const wchar_t* lbl_names[] = {L"None", L"Sparse", L"Full"};
                for (int i = 0; i < 3; ++i) {
                    RECT ar = p->rects.analog_labels[i];
                    ar.top += sdy; ar.bottom += sdy;
                    paint_option_btn(hdc, ar, lbl_names[i],
                                     (int)p->analog_style.hour_labels == i, s);
                }

                RECT color_lbl = map_dlu(dlg, 158, 112, 80, 8);
                color_lbl.top += sdy; color_lbl.bottom += sdy;
                s.draw_label(hdc, color_lbl, L"Colors");
                for (int i = 0; i < ANALOG_COLOR_COUNT; ++i) {
                    RECT r = p->rects.analog_colors[i];
                    r.top += sdy; r.bottom += sdy;
                    int* field = analog_color_field(p->analog_style, i);
                    bool custom = field && *field >= 0;
                    paint_option_btn(hdc, r, analog_color_label(i), custom, s);
                    RECT sw = {r.right - s.scale(14), r.top + s.scale(2), r.right - s.scale(3), r.bottom - s.scale(2)};
                    COLORREF sw_color = custom ? (COLORREF)*field : s.theme->dim;
                    GdiObj br{CreateSolidBrush(sw_color)};
                    FillRect(hdc, &sw, (HBRUSH)br.h);
                }

                RECT value_lbl = map_dlu(dlg, 68, 248, 200, 8);
                value_lbl.top += sdy; value_lbl.bottom += sdy;
                s.draw_label(hdc, value_lbl, L"Size and opacity");
                for (int i = 0; i < ANALOG_VALUE_COUNT; ++i) {
                    RECT r = p->rects.analog_values[i];
                    r.top += sdy; r.bottom += sdy;
                    int* field = analog_value_field(p->analog_style, i);
                    wchar_t buf[96];
                    wsprintfW(buf, L"%s: %d", analog_value_label(i), field ? *field : 0);
                    paint_option_btn(hdc, r, buf, false, s);
                }

                RestoreDC(hdc, sdc2);
            }

            // Fixed preview drawn on top — no sdy offset
            RECT prev_lbl = map_dlu(dlg, 68, 64, 80, 8);
            s.draw_label(hdc, prev_lbl, L"Live preview");
            draw_analog_clock(hdc, preview, p->analog_style, *s.theme, s.dpi, 10, 10, 30);
        }

    } else if (p->active_tab == TAB_TIMERS) {
        RECT lbl = map_dlu(dlg, 70, 28, 100, 12);
        lbl.top += sdy; lbl.bottom += sdy;
        s.draw_label(hdc, lbl, L"Custom presets");

        for (int i = 0; i < 5; ++i) {
            RECT num = map_dlu(dlg, 76, (short)(40 + i * 16), 16, 12);
            num.top += sdy; num.bottom += sdy;
            auto txt = std::format(L"{}.", i + 1);
            s.draw_label(hdc, num, txt.c_str());
        }
    }

    RestoreDC(hdc, save_dc);
    return TRUE;
}

static INT_PTR on_ctl_color(HWND dlg, UINT msg, HDC hdc) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    return dialog_handle_ctl_color(msg, hdc, *p->style.theme, p->brushes);
}

static INT_PTR on_nc_hit_test(HWND dlg, LPARAM lp) {
    if (auto r = dialog_handle_caption_hittest(dlg, lp, TITLE_H + 2)) {
        SetWindowLongPtrW(dlg, DWLP_MSGRESULT, r);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR on_draw_item(HWND dlg, LPARAM lp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    auto* di = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
    if (di->CtlType != ODT_BUTTON) return FALSE;

    if (di->CtlID == IDC_SET_OK || di->CtlID == IDC_SET_CANCEL) {
        const wchar_t* label = (di->CtlID == IDC_SET_OK) ? L"Apply" : L"Cancel";
        bool focused = (di->itemState & ODS_FOCUS) != 0;
        p->style.draw_button(di->hDC, di->rcItem, label, focused);
        return TRUE;
    }

    bool is_radio = (di->CtlID == IDC_THEME_AUTO || di->CtlID == IDC_THEME_LIGHT || di->CtlID == IDC_THEME_DARK);
    bool checked = SendMessage(di->hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED;
    wchar_t ctrl_label[64] = {};
    GetWindowTextW(di->hwndItem, ctrl_label, 63);
    p->style.draw_check_radio(di->hDC, di->rcItem, ctrl_label, checked, is_radio);
    return TRUE;
}

static INT_PTR on_left_button_down(HWND dlg, LPARAM lp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

    for (int i = 0; i < TAB_COUNT; ++i) {
        if (PtInRect(&p->rects.sidebar[i], pt) && i != p->active_tab) {
            set_active_tab(dlg, *p, i);
            return TRUE;
        }
    }

    POINT spt = {pt.x, pt.y + p->scroll_y};

    if (p->active_tab == TAB_CLOCK && p->clock_view == ClockView::Analog) {
        if (PtInRect(&p->rects.analog_min_ticks, spt)) {
            p->analog_style.show_minute_ticks = !p->analog_style.show_minute_ticks;
            InvalidateRect(dlg, nullptr, TRUE);
            return TRUE;
        }
        for (int i = 0; i < 3; ++i) {
            if (PtInRect(&p->rects.analog_labels[i], spt)) {
                p->analog_style.hour_labels = (HourLabels)i;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }
        for (int i = 0; i < ANALOG_COLOR_COUNT; ++i) {
            if (PtInRect(&p->rects.analog_colors[i], spt)) {
                int* field = analog_color_field(p->analog_style, i);
                if (field) {
                    int next = (analog_palette_index(*field) + 1) % ANALOG_PALETTE_COUNT;
                    *field = (int)analog_palette_color(next);
                    InvalidateRect(dlg, nullptr, TRUE);
                    return TRUE;
                }
            }
        }
        for (int i = 0; i < ANALOG_VALUE_COUNT; ++i) {
            if (PtInRect(&p->rects.analog_values[i], spt)) {
                int* field = analog_value_field(p->analog_style, i);
                if (field) {
                    int min_v, max_v, step;
                    analog_value_range(i, min_v, max_v, step);
                    *field += step;
                    if (*field > max_v) *field = min_v;
                    InvalidateRect(dlg, nullptr, TRUE);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static INT_PTR on_scroll(HWND dlg, WPARAM wp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0};
    GetScrollInfo(dlg, SB_VERT, &si);
    int new_pos = si.nPos;
    switch (LOWORD(wp)) {
    case SB_LINEUP:     new_pos -= 10; break;
    case SB_LINEDOWN:   new_pos += 10; break;
    case SB_PAGEUP:     new_pos -= (int)si.nPage; break;
    case SB_PAGEDOWN:   new_pos += (int)si.nPage; break;
    case SB_THUMBTRACK: new_pos = si.nTrackPos; break;
    case SB_TOP:        new_pos = si.nMin; break;
    case SB_BOTTOM:     new_pos = si.nMax; break;
    }
    apply_scroll(dlg, p, new_pos);
    return TRUE;
}

static INT_PTR on_mouse_wheel(HWND dlg, WPARAM wp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0};
    GetScrollInfo(dlg, SB_VERT, &si);
    if ((int)(si.nMax - si.nPage) <= si.nMin) return FALSE;

    int delta = GET_WHEEL_DELTA_WPARAM(wp);
    apply_scroll(dlg, p, si.nPos + (delta > 0 ? -30 : 30));
    return TRUE;
}

static void tab_appearance_commit(HWND dlg, Params& p) {
    for (auto& e : kThemeMap) {
        if (IsDlgButtonChecked(dlg, e.id) == BST_CHECKED) {
            p.theme_mode = e.mode;
            break;
        }
    }
    p.sound_on_expiry = IsDlgButtonChecked(dlg, IDC_SOUND) == BST_CHECKED;
}

static bool tab_pomodoro_commit(HWND dlg, Params& p) {
    int w = 0, s = 0, l = 0, cad = 0;
    if (!read_field(dlg, IDC_POMO_WORK, w) ||
        !read_field(dlg, IDC_POMO_SHORT, s) ||
        !read_field(dlg, IDC_POMO_LONG, l)) {
        set_active_tab(dlg, p, TAB_POMODORO);
        MessageBeep(MB_ICONASTERISK);
        int dummy = 0;
        if (!read_field(dlg, IDC_POMO_WORK, dummy))
            SetFocus(GetDlgItem(dlg, IDC_POMO_WORK));
        else if (!read_field(dlg, IDC_POMO_SHORT, dummy))
            SetFocus(GetDlgItem(dlg, IDC_POMO_SHORT));
        else
            SetFocus(GetDlgItem(dlg, IDC_POMO_LONG));
        return false;
    }

    if (!read_field(dlg, IDC_POMO_CADENCE, cad) ||
        cad < POMODORO_MIN_CADENCE || cad > POMODORO_MAX_CADENCE) {
        set_active_tab(dlg, p, TAB_POMODORO);
        MessageBeep(MB_ICONASTERISK);
        SetFocus(GetDlgItem(dlg, IDC_POMO_CADENCE));
        return false;
    }

    p.work_min = w;
    p.short_min = s;
    p.long_min = l;
    p.cadence = cad;
    p.auto_start = IsDlgButtonChecked(dlg, IDC_AUTO_START) == BST_CHECKED;
    return true;
}

static bool tab_timers_commit(HWND dlg, Params& p) {
    int pr_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                    IDC_PRESET_ED3, IDC_PRESET_ED4};
    for (int i = 0; i < 5; ++i) {
        wchar_t buf[8] = {};
        GetDlgItemTextW(dlg, pr_ids[i], buf, 7);
        if (buf[0] == L'\0') {
            p.preset_min[i] = 0;
            continue;
        }
        wchar_t* end = nullptr;
        long v = std::wcstol(buf, &end, 10);
        p.preset_min[i] = (v >= 1 && v <= 1440) ? (int)v : 0;
    }
    return true;
}

static bool commit_all_tabs(HWND dlg, Params& p) {
    if (!tab_pomodoro_commit(dlg, p) || !tab_timers_commit(dlg, p))
        return false;
    tab_appearance_commit(dlg, p);
    return true;
}

static INT_PTR on_command(HWND dlg, WPARAM wp) {
    switch (LOWORD(wp)) {
    case IDC_CLOCK_COMBO:
        if (HIWORD(wp) == CBN_SELCHANGE) {
            auto* p = dialog_params(dlg);
            if (!p) return FALSE;
            HWND combo = GetDlgItem(dlg, IDC_CLOCK_COMBO);
            int sel = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < CLOCK_VIEW_COUNT) {
                p->clock_view = (ClockView)sel;
                update_content_scroll(dlg, p);
                InvalidateRect(dlg, nullptr, TRUE);
            }
        }
        return TRUE;
    case IDC_THEME_AUTO:
    case IDC_THEME_LIGHT:
    case IDC_THEME_DARK:
        if (HIWORD(wp) == BN_CLICKED) {
            CheckRadioButton(dlg, IDC_THEME_AUTO, IDC_THEME_DARK, LOWORD(wp));
            for (auto& e : kThemeMap)
                InvalidateRect(GetDlgItem(dlg, e.id), nullptr, FALSE);
        }
        return TRUE;
    case IDC_SOUND:
        if (HIWORD(wp) == BN_CLICKED) {
            UINT cur = IsDlgButtonChecked(dlg, IDC_SOUND);
            CheckDlgButton(dlg, IDC_SOUND, cur == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
            InvalidateRect(GetDlgItem(dlg, IDC_SOUND), nullptr, FALSE);
        }
        return TRUE;
    case IDC_AUTO_START:
        if (HIWORD(wp) == BN_CLICKED) {
            UINT cur = IsDlgButtonChecked(dlg, IDC_AUTO_START);
            CheckDlgButton(dlg, IDC_AUTO_START, cur == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
            InvalidateRect(GetDlgItem(dlg, IDC_AUTO_START), nullptr, FALSE);
        }
        return TRUE;
    case IDC_SET_OK: {
        if (HIWORD(wp) != BN_CLICKED) return FALSE;
        auto* p = dialog_params(dlg);
        if (!p) return FALSE;
        if (!commit_all_tabs(dlg, *p)) return TRUE;
        EndDialog(dlg, IDOK);
        return TRUE;
    }
    case IDC_SET_CANCEL:
        if (HIWORD(wp) != BN_CLICKED) return FALSE;
        EndDialog(dlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR on_right_button_down(HWND dlg, LPARAM lp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;
    if (p->active_tab != TAB_CLOCK || p->clock_view != ClockView::Analog) return FALSE;

    POINT spt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp) + p->scroll_y};

    for (int i = 0; i < ANALOG_COLOR_COUNT; ++i) {
        if (PtInRect(&p->rects.analog_colors[i], spt)) {
            int* field = analog_color_field(p->analog_style, i);
            if (field) {
                *field = -1;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }
    }
    for (int i = 0; i < ANALOG_VALUE_COUNT; ++i) {
        if (PtInRect(&p->rects.analog_values[i], spt)) {
            int* field = analog_value_field(p->analog_style, i);
            if (field) {
                int min_v, max_v, step;
                analog_value_range(i, min_v, max_v, step);
                *field -= step;
                if (*field < min_v) *field = max_v;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static INT_PTR on_key_down(HWND dlg, WPARAM wp) {
    if (wp == VK_ESCAPE) {
        EndDialog(dlg, IDCANCEL);
        return TRUE;
    }
    if (wp == VK_RETURN) {
        SendMessageW(dlg, WM_COMMAND, IDC_SET_OK, 0);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG:       return on_init(dlg, lp);
    case WM_SIZE:             return on_resize(dlg, wp);
    case WM_GETMINMAXINFO:    return on_get_min_max_info(dlg, lp);
    case WM_ERASEBKGND:       return on_erase_background(dlg, (HDC)wp);
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORLISTBOX:  return on_ctl_color(dlg, msg, (HDC)wp);
    case WM_NCHITTEST:        return on_nc_hit_test(dlg, lp);
    case WM_DRAWITEM:         return on_draw_item(dlg, lp);
    case WM_LBUTTONDOWN:      return on_left_button_down(dlg, lp);
    case WM_RBUTTONDOWN:      return on_right_button_down(dlg, lp);
    case WM_VSCROLL:          return on_scroll(dlg, wp);
    case WM_MOUSEWHEEL:       return on_mouse_wheel(dlg, wp);
    case WM_COMMAND:          return on_command(dlg, wp);
    case WM_KEYDOWN:          return on_key_down(dlg, wp);
    }
    return FALSE;
}

} // namespace settings_dlg

// ─── Public API ───────────────────────────────────────────────────────────────

bool show_settings_dialog(HWND parent, App& app, HFONT font,
                          const Theme* theme, int dpi) {
    using namespace settings_dlg;
    if (!theme) theme = &dark_theme;
    if (dpi <= 0) dpi = STANDARD_DPI;

    Params p{
        .active_tab    = TAB_APPEARANCE,
        .theme_mode    = app.theme_mode,
        .clock_view    = app.clock_view,
        .analog_style  = app.analog_style,
        .sound_on_expiry = app.sound_on_expiry,
        .work_min  = app.pomodoro_work_secs / 60,
        .short_min = app.pomodoro_short_secs / 60,
        .long_min  = app.pomodoro_long_secs / 60,
        .cadence   = app.pomodoro_cadence,
        .auto_start = app.pomodoro_auto_start,
        .preset_min = {},
        .style = {.theme = theme, .font = font, .dpi = dpi},
        .brushes = {},
    };
    for (int i = 0; i < (int)app.custom_preset_secs.size() && i < 5; ++i)
        p.preset_min[i] = app.custom_preset_secs[i] / 60;

    auto tmpl = build_template();
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    INT_PTR result = DialogBoxIndirectParamW(
        hInst, reinterpret_cast<DLGTEMPLATE*>(tmpl.data()),
        parent, DlgProc, (LPARAM)&p
    );

    if (result != IDOK) return false;
    app.theme_mode       = p.theme_mode;
    app.clock_view       = p.clock_view;
    app.analog_style     = p.analog_style;
    app.sound_on_expiry  = p.sound_on_expiry;
    app.pomodoro_work_secs  = p.work_min * 60;
    app.pomodoro_short_secs = p.short_min * 60;
    app.pomodoro_long_secs  = p.long_min * 60;
    app.pomodoro_cadence    = p.cadence;
    app.pomodoro_auto_start = p.auto_start;
    app.custom_preset_secs.clear();
    for (int i = 0; i < 5; ++i)
        if (p.preset_min[i] > 0)
            app.custom_preset_secs.push_back(p.preset_min[i] * 60);
    return true;
}

} // namespace ui
