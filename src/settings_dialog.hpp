#pragma once
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <format>
#include <vector>
#include "app.hpp"
#include "dialog_style.hpp"

namespace settings_dlg {

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
constexpr int IDC_LBL_CADENCE = 311;
constexpr int IDC_MIN_CADENCE = 312;
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

constexpr WORD CLS_BUTTON = 0x0080;
constexpr WORD CLS_EDIT   = 0x0081;
constexpr WORD CLS_STATIC = 0x0082;

constexpr short DLG_W = 240, DLG_H = 156;
constexpr short SIDEBAR_W = 62;
constexpr short TITLE_H = 18;

constexpr int TAB_APPEARANCE = 0;
constexpr int TAB_CLOCK      = 1;
constexpr int TAB_POMODORO   = 2;
constexpr int TAB_TIMERS     = 3;
constexpr int TAB_COUNT      = 4;

struct Buf {
    std::vector<WORD> data;
    void align4() { while (data.size() % 2) data.push_back(0); }
    void push_word(WORD w)  { data.push_back(w); }
    void push_dword(DWORD d) { data.push_back(LOWORD(d)); data.push_back(HIWORD(d)); }
    void push_wstr(const wchar_t* s) { while (*s) data.push_back((WORD)*s++); data.push_back(0); }
    void push_wstr_empty() { data.push_back(0); }
};

inline void add_item(Buf& b, DWORD style, DWORD exStyle,
                     short x, short y, short cx, short cy,
                     WORD id, WORD cls_atom, const wchar_t* title) {
    b.align4();
    b.push_dword(WS_CHILD | WS_VISIBLE | style);
    b.push_dword(exStyle);
    b.push_word((WORD)x); b.push_word((WORD)y);
    b.push_word((WORD)cx); b.push_word((WORD)cy);
    b.push_word(id);
    b.push_word(0xFFFF); b.push_word(cls_atom);
    b.push_wstr(title);
    b.push_word(0);
}

constexpr WORD CTRL_COUNT = 25;

inline std::vector<WORD> build_template() {
    Buf b;
    b.push_dword(WS_POPUP | WS_BORDER);
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
    short row_h = 12;
    short y0 = 40, sp = 18;

    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, y0, lbl_w, row_h, IDC_LBL_WORK, CLS_STATIC, L"Work");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, y0, ed_w, row_h, IDC_POMO_WORK, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, y0, min_w, row_h, IDC_MIN_WORK, CLS_STATIC, L"min");

    short r1 = y0 + sp;
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, r1, lbl_w, row_h, IDC_LBL_SHORT, CLS_STATIC, L"Short break");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, r1, ed_w, row_h, IDC_POMO_SHORT, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, r1, min_w, row_h, IDC_MIN_SHORT, CLS_STATIC, L"min");

    short r2 = y0 + 2 * sp;
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, r2, lbl_w, row_h, IDC_LBL_LONG, CLS_STATIC, L"Long break");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, r2, ed_w, row_h, IDC_POMO_LONG, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, r2, min_w, row_h, IDC_MIN_LONG, CLS_STATIC, L"min");

    short r3 = y0 + 3 * sp;
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, r3, lbl_w, row_h, IDC_LBL_CADENCE, CLS_STATIC, L"Long every");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, r3, ed_w, row_h, IDC_POMO_CADENCE, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, r3, min_w + 20, row_h, IDC_MIN_CADENCE, CLS_STATIC, L"sessions");

    short pr_ed_x = 96, pr_ed_w = 28;
    short pr_min_x = 128, pr_min_w = 18;
    short pr_y0 = 40, pr_sp = 16;
    int pr_edit_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                         IDC_PRESET_ED3, IDC_PRESET_ED4};
    int pr_min_ids[] = {IDC_PRESET_MIN0, IDC_PRESET_MIN1, IDC_PRESET_MIN2,
                        IDC_PRESET_MIN3, IDC_PRESET_MIN4};
    for (int i = 0; i < 5; ++i) {
        short py = pr_y0 + (short)(i * pr_sp);
        add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
                 pr_ed_x, py, pr_ed_w, row_h, (WORD)pr_edit_ids[i], CLS_EDIT, L"");
        add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
                 pr_min_x, py, pr_min_w, row_h, (WORD)pr_min_ids[i], CLS_STATIC, L"min");
    }

    // Clock format combobox (hidden by default, shown on TAB_CLOCK)
    // Height 80 DLU gives enough room for the drop-down list
    add_item(b, CBS_DROPDOWNLIST | WS_TABSTOP, 0,
             70, 40, 100, 80, IDC_CLOCK_COMBO, 0x0085 /*COMBOBOX*/, L"");

    short btn_w = 44, btn_h = 16, btn_gap = 8;
    short content_cx = DLG_W - SIDEBAR_W - 4;
    short total_bw = 2 * btn_w + btn_gap;
    short btn_x0 = SIDEBAR_W + 2 + (content_cx - total_bw) / 2;
    short btn_y = 138;
    add_item(b, BS_OWNERDRAW | WS_TABSTOP, 0,
             btn_x0, btn_y, btn_w, btn_h, IDC_SET_OK, CLS_BUTTON, L"OK");
    add_item(b, BS_OWNERDRAW | WS_TABSTOP, 0,
             btn_x0 + btn_w + btn_gap, btn_y, btn_w, btn_h, IDC_SET_CANCEL, CLS_BUTTON, L"Cancel");

    return b.data;
}

inline RECT map_dlu(HWND dlg, short x, short y, short cx, short cy) {
    RECT r = {x, y, x + cx, y + cy};
    MapDialogRect(dlg, &r);
    return r;
}

struct HitRects {
    RECT sidebar[TAB_COUNT];
    RECT theme[3];
    RECT sound;
    RECT auto_start;
    RECT preset_row[5];
    RECT analog_min_ticks;
    RECT analog_labels[3];
};

inline HitRects compute_rects(HWND dlg) {
    HitRects h{};
    h.sidebar[0] = map_dlu(dlg, 3, 24, 56, 14);
    h.sidebar[1] = map_dlu(dlg, 3, 40, 56, 14);
    h.sidebar[2] = map_dlu(dlg, 3, 56, 56, 14);
    h.sidebar[3] = map_dlu(dlg, 3, 72, 56, 14);

    h.theme[0] = map_dlu(dlg, 70, 42, 54, 13);
    h.theme[1] = map_dlu(dlg, 70, 57, 54, 13);
    h.theme[2] = map_dlu(dlg, 70, 72, 54, 13);

    // Analog sub-options: combobox sits at y=40..53, options start at y=60
    h.analog_min_ticks = map_dlu(dlg, 70, 72, 80, 12);
    h.analog_labels[0] = map_dlu(dlg, 70, 90, 34, 12);
    h.analog_labels[1] = map_dlu(dlg, 106, 90, 34, 12);
    h.analog_labels[2] = map_dlu(dlg, 142, 90, 34, 12);

    h.sound = map_dlu(dlg, 70, 97, 100, 13);
    h.auto_start = map_dlu(dlg, 70, 115, 100, 13);
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
    int scroll_offset = 0;  // content scroll in pixels
};

inline void set_pomo_visible(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    for (int id = IDC_POMO_WORK; id <= IDC_MIN_CADENCE; ++id) {
        HWND h = GetDlgItem(dlg, id);
        if (h) ShowWindow(h, cmd);
    }
}

inline void set_presets_visible(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    int ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2, IDC_PRESET_ED3, IDC_PRESET_ED4,
                 IDC_PRESET_MIN0, IDC_PRESET_MIN1, IDC_PRESET_MIN2, IDC_PRESET_MIN3, IDC_PRESET_MIN4};
    for (int id : ids) {
        HWND h = GetDlgItem(dlg, id);
        if (h) ShowWindow(h, cmd);
    }
}

inline void set_clock_combo_visible(HWND dlg, bool show) {
    HWND h = GetDlgItem(dlg, IDC_CLOCK_COMBO);
    if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
}

// Returns the virtual content height (pixels) for the given tab to drive scrollbar range.
inline int content_height_for_tab(HWND dlg, int tab, ClockView cv) {
    if (tab == TAB_CLOCK && cv == ClockView::Analog) {
        RECT r = map_dlu(dlg, 0, 0, 0, 108);
        return r.bottom;
    }
    RECT r = map_dlu(dlg, 0, 0, 0, 100);
    return r.bottom;
}

inline void update_clock_scrollbar(HWND dlg, Params* p) {
    RECT title_div = {0, 0, 0, TITLE_H + 2};
    MapDialogRect(dlg, &title_div);
    RECT btn_div_rc = {0, 0, 0, 132};
    MapDialogRect(dlg, &btn_div_rc);
    int view_h = btn_div_rc.bottom - title_div.bottom;
    int virtual_h = content_height_for_tab(dlg, p->active_tab, p->clock_view);

    if (virtual_h <= view_h) {
        p->scroll_offset = 0;
        ShowScrollBar(dlg, SB_VERT, FALSE);
        return;
    }

    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = virtual_h;
    si.nPage  = view_h;
    si.nPos   = p->scroll_offset;
    SetScrollInfo(dlg, SB_VERT, &si, TRUE);
    ShowScrollBar(dlg, SB_VERT, TRUE);
}

inline void paint_option_btn(HDC hdc, const RECT& rc, const wchar_t* text, bool selected,
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

inline bool read_field(HWND dlg, int id, int& out) {
    wchar_t buf[8] = {};
    GetDlgItemTextW(dlg, id, buf, 7);
    wchar_t* end = nullptr;
    long v = std::wcstol(buf, &end, 10);
    if (!end || *end != L'\0' || v < 1 || v > 1440) return false;
    out = (int)v;
    return true;
}

inline INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        auto* p = reinterpret_cast<Params*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)p);
        p->style.apply_dark_mode(dlg);

        HWND parent = GetParent(dlg);
        if (parent) {
            RECT pr, dr;
            GetWindowRect(parent, &pr);
            GetWindowRect(dlg, &dr);
            int dw = dr.right - dr.left, dh = dr.bottom - dr.top;
            int x = pr.left + (pr.right - pr.left - dw) / 2;
            int y = pr.top + (pr.bottom - pr.top - dh) / 2;
            SetWindowPos(dlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }

        p->rects = compute_rects(dlg);

        SetDlgItemTextW(dlg, IDC_POMO_WORK,  std::format(L"{}", p->work_min).c_str());
        SetDlgItemTextW(dlg, IDC_POMO_SHORT, std::format(L"{}", p->short_min).c_str());
        SetDlgItemTextW(dlg, IDC_POMO_LONG,  std::format(L"{}", p->long_min).c_str());
        SetDlgItemTextW(dlg, IDC_POMO_CADENCE, std::format(L"{}", p->cadence).c_str());
        SendDlgItemMessageW(dlg, IDC_POMO_WORK,  EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_POMO_SHORT, EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_POMO_LONG,  EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_POMO_CADENCE, EM_SETLIMITTEXT, 2, 0);

        int pr_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                        IDC_PRESET_ED3, IDC_PRESET_ED4};
        for (int i = 0; i < 5; ++i) {
            if (p->preset_min[i] > 0)
                SetDlgItemTextW(dlg, pr_ids[i], std::format(L"{}", p->preset_min[i]).c_str());
            SendDlgItemMessageW(dlg, pr_ids[i], EM_SETLIMITTEXT, 4, 0);
        }

        EnumChildWindows(dlg, [](HWND child, LPARAM param) -> BOOL {
            auto* params = reinterpret_cast<Params*>(param);
            SendMessageW(child, WM_SETFONT, (WPARAM)params->style.font, FALSE);
            return TRUE;
        }, (LPARAM)p);

        // Populate clock format combobox
        {
            HWND hc = GetDlgItem(dlg, IDC_CLOCK_COMBO);
            const wchar_t* fmt_names[] = {
                L"24h + seconds", L"24h", L"12h + seconds", L"12h", L"Analog"
            };
            for (int i = 0; i < CLOCK_VIEW_COUNT; ++i)
                SendMessageW(hc, CB_ADDSTRING, 0, (LPARAM)fmt_names[i]);
            SendMessageW(hc, CB_SETCURSEL, (WPARAM)(int)p->clock_view, 0);
        }

        set_pomo_visible(dlg, p->active_tab == TAB_POMODORO);
        set_presets_visible(dlg, p->active_tab == TAB_TIMERS);
        set_clock_combo_visible(dlg, p->active_tab == TAB_CLOCK);
        if (p->active_tab == TAB_CLOCK) update_clock_scrollbar(dlg, p);
        return FALSE;
    }

    case WM_ERASEBKGND: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        HDC hdc = (HDC)wp;
        RECT rc;
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

        RECT btn_div = {0, 0, 0, 132};
        MapDialogRect(dlg, &btn_div);
        MoveToEx(hdc, sb.right, btn_div.bottom, nullptr);
        LineTo(hdc, rc.right, btn_div.bottom);

        SelectObject(hdc, old_pen);
        DeleteObject(div_pen);

        const wchar_t* tab_names[] = {L"Appearance", L"Clock", L"Pomodoro", L"Timers"};
        for (int i = 0; i < TAB_COUNT; ++i)
            paint_option_btn(hdc, p->rects.sidebar[i], tab_names[i],
                            i == p->active_tab, s);

        if (p->active_tab == TAB_APPEARANCE) {
            RECT lbl = map_dlu(dlg, 70, 28, 80, 12);
            s.draw_label(hdc, lbl, L"Theme");
            const wchar_t* names[] = {L"Auto", L"Light", L"Dark"};
            for (int i = 0; i < 3; ++i)
                paint_option_btn(hdc, p->rects.theme[i], names[i],
                                (int)p->theme_mode == i, s);

            RECT slbl = map_dlu(dlg, 70, 88, 80, 8);
            s.draw_label(hdc, slbl, L"Notifications");
            paint_option_btn(hdc, p->rects.sound,
                            p->sound_on_expiry ? L"Sound: on" : L"Sound: off",
                            p->sound_on_expiry, s);
        } else if (p->active_tab == TAB_POMODORO) {
            RECT aslbl = map_dlu(dlg, 70, 105, 80, 8);
            s.draw_label(hdc, aslbl, L"Auto-start");
            paint_option_btn(hdc, p->rects.auto_start,
                            p->auto_start ? L"On" : L"Off",
                            p->auto_start, s);
        } else if (p->active_tab == TAB_CLOCK) {
            int so = p->scroll_offset;
            auto shifted = [&](RECT r) -> RECT {
                OffsetRect(&r, 0, -so);
                return r;
            };
            RECT lbl = map_dlu(dlg, 70, 28, 80, 12);
            s.draw_label(hdc, shifted(lbl), L"Format");

            if (p->clock_view == ClockView::Analog) {
                RECT aslbl = map_dlu(dlg, 70, 60, 90, 10);
                s.draw_label(hdc, shifted(aslbl), L"Analog options");
                paint_option_btn(hdc, shifted(p->rects.analog_min_ticks),
                                p->analog_style.show_minute_ticks ? L"Min ticks: on" : L"Min ticks: off",
                                p->analog_style.show_minute_ticks, s);
                RECT hrlbl = map_dlu(dlg, 70, 86, 90, 8);
                s.draw_label(hdc, shifted(hrlbl), L"Hour labels");
                const wchar_t* lbl_names[] = {L"None", L"Sparse", L"Full"};
                for (int i = 0; i < 3; ++i)
                    paint_option_btn(hdc, shifted(p->rects.analog_labels[i]), lbl_names[i],
                                    (int)p->analog_style.hour_labels == i, s);
            }
        } else if (p->active_tab == TAB_TIMERS) {
            RECT lbl = map_dlu(dlg, 70, 28, 100, 12);
            s.draw_label(hdc, lbl, L"Custom presets");
            for (int i = 0; i < 5; ++i) {
                RECT num = map_dlu(dlg, 76, (short)(40 + i * 16), 16, 12);
                auto txt = std::format(L"{}.", i + 1);
                s.draw_label(hdc, num, txt.c_str());
            }
        }

        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, p->style.theme->text);
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH s_bg = nullptr;
        if (s_bg) DeleteObject(s_bg);
        s_bg = CreateSolidBrush(p->style.theme->bg);
        return (INT_PTR)s_bg;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, p->style.theme->text);
        SetBkColor(hdc, p->style.theme->bar);
        static HBRUSH s_edit_bg = nullptr;
        if (s_edit_bg) DeleteObject(s_edit_bg);
        s_edit_bg = CreateSolidBrush(p->style.theme->bar);
        return (INT_PTR)s_edit_bg;
    }

    case WM_CTLCOLORBTN: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        static HBRUSH s_btn_bg = nullptr;
        if (s_btn_bg) DeleteObject(s_btn_bg);
        s_btn_bg = CreateSolidBrush(p->style.theme->bg);
        return (INT_PTR)s_btn_bg;
    }

    case WM_NCHITTEST: {
        RECT title_rc = {0, 0, DLG_W, TITLE_H + 2};
        MapDialogRect(dlg, &title_rc);
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(dlg, &pt);
        if (pt.y >= 0 && pt.y < title_rc.bottom)
            return HTCAPTION;
        break;
    }

    case WM_DRAWITEM: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        auto* di = (DRAWITEMSTRUCT*)lp;
        if (di->CtlType != ODT_BUTTON) break;
        const wchar_t* label = (di->CtlID == IDC_SET_OK) ? L"OK" : L"Cancel";
        bool focused = (di->itemState & ODS_FOCUS) != 0;
        p->style.draw_button(di->hDC, di->rcItem, label, focused);
        return TRUE;
    }

    case WM_LBUTTONDOWN: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

        for (int i = 0; i < TAB_COUNT; ++i) {
            if (PtInRect(&p->rects.sidebar[i], pt) && i != p->active_tab) {
                set_pomo_visible(dlg, false);
                set_presets_visible(dlg, false);
                set_clock_combo_visible(dlg, false);
                p->active_tab = i;
                p->scroll_offset = 0;
                set_pomo_visible(dlg, i == TAB_POMODORO);
                set_presets_visible(dlg, i == TAB_TIMERS);
                set_clock_combo_visible(dlg, i == TAB_CLOCK);
                if (i == TAB_CLOCK) update_clock_scrollbar(dlg, p);
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }

        if (p->active_tab == TAB_APPEARANCE) {
            for (int i = 0; i < 3; ++i) {
                if (PtInRect(&p->rects.theme[i], pt)) {
                    p->theme_mode = (ThemeMode)i;
                    InvalidateRect(dlg, nullptr, TRUE);
                    return TRUE;
                }
            }
            if (PtInRect(&p->rects.sound, pt)) {
                p->sound_on_expiry = !p->sound_on_expiry;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }

        if (p->active_tab == TAB_CLOCK && p->clock_view == ClockView::Analog) {
            // Adjust hit point for scroll offset
            POINT spt = {pt.x, pt.y + p->scroll_offset};
            RECT ticks = p->rects.analog_min_ticks;
            if (PtInRect(&ticks, spt)) {
                p->analog_style.show_minute_ticks = !p->analog_style.show_minute_ticks;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
            for (int i = 0; i < 3; ++i) {
                RECT lr = p->rects.analog_labels[i];
                if (PtInRect(&lr, spt)) {
                    p->analog_style.hour_labels = (HourLabels)i;
                    InvalidateRect(dlg, nullptr, TRUE);
                    return TRUE;
                }
            }
        }

        if (p->active_tab == TAB_POMODORO) {
            if (PtInRect(&p->rects.auto_start, pt)) {
                p->auto_start = !p->auto_start;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }

        break;
    }

    case WM_VSCROLL: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p || p->active_tab != TAB_CLOCK) break;
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(dlg, SB_VERT, &si);
        int prev = si.nPos;
        switch (LOWORD(wp)) {
        case SB_LINEUP:    si.nPos -= 8; break;
        case SB_LINEDOWN:  si.nPos += 8; break;
        case SB_PAGEUP:    si.nPos -= si.nPage; break;
        case SB_PAGEDOWN:  si.nPos += si.nPage; break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        }
        si.nPos = std::clamp(si.nPos, si.nMin, (int)(si.nMax - si.nPage));
        if (si.nPos != prev) {
            si.fMask = SIF_POS;
            SetScrollInfo(dlg, SB_VERT, &si, TRUE);
            p->scroll_offset = si.nPos;
            InvalidateRect(dlg, nullptr, TRUE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p || p->active_tab != TAB_CLOCK) break;
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(dlg, SB_VERT, &si);
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        si.nPos -= delta / WHEEL_DELTA * 20;
        si.nPos = std::clamp(si.nPos, si.nMin, (int)(si.nMax - si.nPage));
        si.fMask = SIF_POS;
        SetScrollInfo(dlg, SB_VERT, &si, TRUE);
        p->scroll_offset = si.nPos;
        InvalidateRect(dlg, nullptr, TRUE);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_CLOCK_COMBO && HIWORD(wp) == CBN_SELCHANGE) {
            auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
            if (!p) break;
            int sel = (int)SendDlgItemMessageW(dlg, IDC_CLOCK_COMBO, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR)
                p->clock_view = (ClockView)sel;
            update_clock_scrollbar(dlg, p);
            InvalidateRect(dlg, nullptr, TRUE);
            return TRUE;
        }
        switch (LOWORD(wp)) {
        case IDC_SET_OK: {
            auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
            int w = 0, s = 0, l = 0, cad = 0;
            if (!read_field(dlg, IDC_POMO_WORK, w) ||
                !read_field(dlg, IDC_POMO_SHORT, s) ||
                !read_field(dlg, IDC_POMO_LONG, l)) {
                set_pomo_visible(dlg, true);
                p->active_tab = TAB_POMODORO;
                InvalidateRect(dlg, nullptr, TRUE);
                MessageBeep(MB_ICONASTERISK);
                int dummy;
                if (!read_field(dlg, IDC_POMO_WORK, dummy))
                    SetFocus(GetDlgItem(dlg, IDC_POMO_WORK));
                else if (!read_field(dlg, IDC_POMO_SHORT, dummy))
                    SetFocus(GetDlgItem(dlg, IDC_POMO_SHORT));
                else
                    SetFocus(GetDlgItem(dlg, IDC_POMO_LONG));
                return TRUE;
            }
            if (!read_field(dlg, IDC_POMO_CADENCE, cad) ||
                cad < POMODORO_MIN_CADENCE || cad > POMODORO_MAX_CADENCE) {
                set_pomo_visible(dlg, true);
                p->active_tab = TAB_POMODORO;
                InvalidateRect(dlg, nullptr, TRUE);
                MessageBeep(MB_ICONASTERISK);
                SetFocus(GetDlgItem(dlg, IDC_POMO_CADENCE));
                return TRUE;
            }
            p->work_min = w; p->short_min = s; p->long_min = l;
            p->cadence = cad;
            {
                int pr_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                                IDC_PRESET_ED3, IDC_PRESET_ED4};
                for (int i = 0; i < 5; ++i) {
                    wchar_t buf[8] = {};
                    GetDlgItemTextW(dlg, pr_ids[i], buf, 7);
                    if (buf[0] == L'\0') {
                        p->preset_min[i] = 0;
                        continue;
                    }
                    wchar_t* end = nullptr;
                    long v = std::wcstol(buf, &end, 10);
                    p->preset_min[i] = (v >= 1 && v <= 1440) ? (int)v : 0;
                }
            }
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        case IDC_SET_CANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { EndDialog(dlg, IDCANCEL); return TRUE; }
        if (wp == VK_RETURN) { SendMessageW(dlg, WM_COMMAND, IDC_SET_OK, 0); return TRUE; }
        break;
    }
    return FALSE;
}

} // namespace settings_dlg

inline bool show_settings_dialog(HWND parent, App& app, HFONT font,
                                  const Theme* theme = nullptr, int dpi = 0) {
    using namespace settings_dlg;
    if (!theme) theme = &dark_theme;
    if (dpi <= 0) dpi = STANDARD_DPI;

    Params p{
        .active_tab = TAB_APPEARANCE,
        .theme_mode = app.theme_mode,
        .clock_view = app.clock_view,
        .analog_style = app.analog_style,
        .sound_on_expiry = app.sound_on_expiry,
        .work_min  = app.pomodoro_work_secs / 60,
        .short_min = app.pomodoro_short_secs / 60,
        .long_min  = app.pomodoro_long_secs / 60,
        .cadence   = app.pomodoro_cadence,
        .auto_start = app.pomodoro_auto_start,
        .preset_min = {},
        .style = {.theme = theme, .font = font, .dpi = dpi},
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
    app.theme_mode = p.theme_mode;
    app.clock_view = p.clock_view;
    app.analog_style = p.analog_style;
    app.sound_on_expiry = p.sound_on_expiry;
    app.pomodoro_work_secs = p.work_min * 60;
    app.pomodoro_short_secs = p.short_min * 60;
    app.pomodoro_long_secs = p.long_min * 60;
    app.pomodoro_cadence = p.cadence;
    app.pomodoro_auto_start = p.auto_start;
    app.custom_preset_secs.clear();
    for (int i = 0; i < 5; ++i)
        if (p.preset_min[i] > 0)
            app.custom_preset_secs.push_back(p.preset_min[i] * 60);
    return true;
}
