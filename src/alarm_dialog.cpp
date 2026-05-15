#include "alarm_dialog.hpp"
#include <windows.h>
#include <windowsx.h>
#include <format>
#include <string>
#include "alarm.hpp"
#include "dialog_style.hpp"
#include "encoding.hpp"

namespace alarm_dlg {

// ─── Control IDs ─────────────────────────────────────────────────────────────

constexpr WORD IDC_ALM_NAME   = 401;
constexpr WORD IDC_ALM_HOUR   = 402;
constexpr WORD IDC_ALM_MIN    = 403;
constexpr WORD IDC_ALM_YEAR   = 404;
constexpr WORD IDC_ALM_MONTH  = 405;
constexpr WORD IDC_ALM_DAY    = 406;
constexpr WORD IDC_ALM_OK     = 407;
constexpr WORD IDC_ALM_CANCEL = 408;

constexpr WORD CLS_BUTTON  = 0x0080;
constexpr WORD CLS_EDIT    = 0x0081;

// Dialog dimensions (DLU)
constexpr short DLG_W = 200, DLG_H = 138;
constexpr WORD  CTRL_COUNT = 8;

// ─── Layout helpers ───────────────────────────────────────────────────────────

struct Params {
    Alarm result;
    DlgStyle style;
    // Painted-control rects (pixels, set in WM_INITDIALOG via MapDialogRect)
    RECT rc_days_btn{};
    RECT rc_date_btn{};
    RECT rc_day[7]{};  // Mon-Sun painted toggles
    DialogBrushes brushes;
};

// ─── Template builder ─────────────────────────────────────────────────────────

std::vector<WORD> build_template() {
    DlgBuf b;
    b.push_dword(WS_POPUP | WS_BORDER);
    b.push_dword(0);
    b.push_word(CTRL_COUNT);
    b.push_word(0); b.push_word(0);
    b.push_word(DLG_W); b.push_word(DLG_H);
    b.push_wstr_empty(); // menu
    b.push_wstr_empty(); // class
    b.push_wstr_empty(); // caption (painted)

    // Name edit  (x=46 y=25 w=148 h=11)
    dlg_add_item(b, ES_AUTOHSCROLL | WS_TABSTOP, 46, 25, 148, 11, IDC_ALM_NAME, CLS_EDIT, L"");
    // Hour edit  (x=46 y=40 w=22 h=11)
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 46, 40, 22, 11, IDC_ALM_HOUR, CLS_EDIT, L"");
    // Min edit   (x=78 y=40 w=22 h=11)
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 78, 40, 22, 11, IDC_ALM_MIN,  CLS_EDIT, L"");

    // Date fields: Year (x=36 y=70 w=38 h=11), Month (x=104 y=70 w=22 h=11), Day (x=158 y=70 w=22 h=11)
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 36, 70, 38, 11, IDC_ALM_YEAR,  CLS_EDIT, L"");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 104, 70, 22, 11, IDC_ALM_MONTH, CLS_EDIT, L"");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 158, 70, 22, 11, IDC_ALM_DAY,   CLS_EDIT, L"");

    // OK / Cancel (owner-draw)
    constexpr short btn_w = 44, btn_h = 16;
    constexpr short btn_y = 118;
    constexpr short btn_x0 = (DLG_W - 2 * btn_w - 8) / 2;
    dlg_add_item(b, BS_OWNERDRAW | WS_TABSTOP, btn_x0, btn_y, btn_w, btn_h, IDC_ALM_OK,     CLS_BUTTON, L"OK");
    dlg_add_item(b, BS_OWNERDRAW | WS_TABSTOP, btn_x0 + btn_w + 8, btn_y, btn_w, btn_h, IDC_ALM_CANCEL, CLS_BUTTON, L"Cancel");

    return b.data;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

static RECT map_dlu(HWND dlg, short x, short y, short cx, short cy) {
    RECT r = {x, y, x + cx, y + cy};
    MapDialogRect(dlg, &r);
    return r;
}

static void show_date_controls(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    for (WORD id : {IDC_ALM_YEAR, IDC_ALM_MONTH, IDC_ALM_DAY})
        if (HWND h = GetDlgItem(dlg, id)) ShowWindow(h, cmd);
}

static void init_painted_rects(HWND dlg, Params& p) {
    // Days / Date toggle buttons (painted)
    p.rc_days_btn = map_dlu(dlg, 8,  56, 44, 13);
    p.rc_date_btn = map_dlu(dlg, 56, 56, 44, 13);

    // Day toggles (Mon=bit0 .. Sun=bit6): row1 Mon-Fri, row2 Sat-Sun
    // Row 1: Mon Tue Wed Thu Fri (x=8,36,64,92,120  y=72  w=26 h=13)
    // Row 2: Sat Sun             (x=8,36              y=87  w=26 h=13)
    static constexpr short xs[] = {8, 36, 64, 92, 120, 8, 36};
    static constexpr short ys[] = {72,72,72, 72,  72, 87, 87};
    for (int i = 0; i < 7; ++i)
        p.rc_day[i] = map_dlu(dlg, xs[i], ys[i], 26, 13);
}

static void paint_toggle_btn(HDC hdc, const RECT& r, const wchar_t* text, bool active,
                              const DlgStyle& s) {
    int rr = s.scale(4);
    HBRUSH br = CreateSolidBrush(active ? s.theme->active : s.theme->btn);
    HPEN   pn = CreatePen(PS_NULL, 0, 0);
    auto* obr = (HBRUSH)SelectObject(hdc, br);
    auto* opn = (HPEN)SelectObject(hdc, pn);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rr, rr);
    SelectObject(hdc, obr); SelectObject(hdc, opn);
    DeleteObject(br); DeleteObject(pn);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, s.theme->text);
    SelectObject(hdc, s.font);
    RECT rc = r;
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ─── Dialog procedure ─────────────────────────────────────────────────────────

INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));

    switch (msg) {
    case WM_INITDIALOG: {
        p = reinterpret_cast<Params*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)p);
        p->style.apply_dark_mode(dlg);
        dialog_center_on_parent(dlg);

        init_painted_rects(dlg, *p);
        p->brushes = DialogBrushes::create(*p->style.theme);
        dialog_apply_font_to_children(dlg, p->style.font);

        // Populate edits
        const auto& a = p->result;
        std::wstring name_w = utf8_to_wide(a.name);
        SetDlgItemTextW(dlg, IDC_ALM_NAME, name_w.c_str());
        SendDlgItemMessageW(dlg, IDC_ALM_NAME, EM_SETLIMITTEXT, 40, 0);
        SetDlgItemTextW(dlg, IDC_ALM_HOUR, std::format(L"{:02}", a.hour).c_str());
        SetDlgItemTextW(dlg, IDC_ALM_MIN,  std::format(L"{:02}", a.minute).c_str());
        SendDlgItemMessageW(dlg, IDC_ALM_HOUR, EM_SETLIMITTEXT, 2, 0);
        SendDlgItemMessageW(dlg, IDC_ALM_MIN,  EM_SETLIMITTEXT, 2, 0);

        SetDlgItemTextW(dlg, IDC_ALM_YEAR,  std::format(L"{}", a.date_year).c_str());
        SetDlgItemTextW(dlg, IDC_ALM_MONTH, std::format(L"{:02}", a.date_month).c_str());
        SetDlgItemTextW(dlg, IDC_ALM_DAY,   std::format(L"{:02}", a.date_day).c_str());
        SendDlgItemMessageW(dlg, IDC_ALM_YEAR,  EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_ALM_MONTH, EM_SETLIMITTEXT, 2, 0);
        SendDlgItemMessageW(dlg, IDC_ALM_DAY,   EM_SETLIMITTEXT, 2, 0);

        // Show/hide date fields based on schedule type
        bool is_date = (a.schedule == AlarmSchedule::Date);
        show_date_controls(dlg, is_date);

        SetFocus(GetDlgItem(dlg, IDC_ALM_NAME));
        return FALSE;
    }

    case WM_ERASEBKGND: {
        if (!p) return FALSE;
        HDC hdc = (HDC)wp;
        RECT rc; GetClientRect(dlg, &rc);
        p->style.fill_background(hdc, rc);
        dialog_paint_title(hdc, dlg, p->style, L"Add Alarm", 21);

        // Field labels
        auto lbl = [&](short x, short y, short cx, short cy, const wchar_t* t) {
            RECT r = {x, y, x + cx, y + cy}; MapDialogRect(dlg, &r);
            p->style.draw_label(hdc, r, t);
        };
        lbl(8, 25, 36, 11, L"Name:");
        lbl(8, 40, 36, 11, L"Time:");
        RECT colon = {70, 40, 76, 51}; MapDialogRect(dlg, &colon);
        p->style.draw_label(hdc, colon, L":", DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        lbl(8, 56, 36, 11, L"Schedule:");

        // Days / Date toggle buttons (painted)
        bool is_days = (p->result.schedule == AlarmSchedule::Days);
        paint_toggle_btn(hdc, p->rc_days_btn, L"Days", is_days,  p->style);
        paint_toggle_btn(hdc, p->rc_date_btn, L"Date", !is_days, p->style);

        if (is_days) {
            // Paint Mon–Sun toggle buttons
            static constexpr const wchar_t* day_names[] = {
                L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat", L"Sun"
            };
            for (int i = 0; i < 7; ++i) {
                bool on = (p->result.days_mask & (1 << i)) != 0;
                paint_toggle_btn(hdc, p->rc_day[i], day_names[i], on, p->style);
            }
        } else {
            // Paint date labels (the edits sit beside them)
            lbl(8,   70, 26, 11, L"Year:");
            lbl(80,  70, 22, 11, L"Mo:");
            lbl(134, 70, 22, 11, L"Dy:");
        }

        return 1;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
        if (!p) break;
        return dialog_handle_ctl_color(msg, (HDC)wp, *p->style.theme, p->brushes);

    case WM_NCHITTEST:
        if (auto r = dialog_handle_caption_hittest(dlg, lp, 21)) return r;
        break;

    case WM_LBUTTONDOWN: {
        if (!p) break;
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        bool was_days = (p->result.schedule == AlarmSchedule::Days);

        if (PtInRect(&p->rc_days_btn, pt) && !was_days) {
            p->result.schedule = AlarmSchedule::Days;
            show_date_controls(dlg, false);
            InvalidateRect(dlg, nullptr, TRUE);
            return TRUE;
        }
        if (PtInRect(&p->rc_date_btn, pt) && was_days) {
            p->result.schedule = AlarmSchedule::Date;
            show_date_controls(dlg, true);
            InvalidateRect(dlg, nullptr, TRUE);
            return TRUE;
        }
        if (was_days) {
            for (int i = 0; i < 7; ++i) {
                if (PtInRect(&p->rc_day[i], pt)) {
                    p->result.days_mask ^= (1 << i);
                    InvalidateRect(dlg, nullptr, TRUE);
                    return TRUE;
                }
            }
        }
        break;
    }

    case WM_DRAWITEM: {
        if (!p) break;
        auto* di = (DRAWITEMSTRUCT*)lp;
        if (di->CtlType != ODT_BUTTON) break;
        bool focused = (di->itemState & ODS_FOCUS) != 0;
        const wchar_t* lbl = (di->CtlID == IDC_ALM_OK) ? L"OK" : L"Cancel";
        p->style.draw_button(di->hDC, di->rcItem, lbl, focused);
        return TRUE;
    }

    case WM_COMMAND: {
        switch (LOWORD(wp)) {
        case IDC_ALM_OK: {
            if (!p) return TRUE;
            // Collect name
            wchar_t nbuf[48] = {};
            GetDlgItemTextW(dlg, IDC_ALM_NAME, nbuf, 47);
            p->result.name = wide_to_utf8(nbuf);

            // Hour
            wchar_t hbuf[4] = {};
            GetDlgItemTextW(dlg, IDC_ALM_HOUR, hbuf, 3);
            long hv = std::wcstol(hbuf, nullptr, 10);
            if (hv < 0 || hv > 23) {
                MessageBeep(MB_ICONASTERISK);
                SetFocus(GetDlgItem(dlg, IDC_ALM_HOUR));
                return TRUE;
            }
            p->result.hour = (int)hv;

            // Minute
            wchar_t mbuf[4] = {};
            GetDlgItemTextW(dlg, IDC_ALM_MIN, mbuf, 3);
            long mv = std::wcstol(mbuf, nullptr, 10);
            if (mv < 0 || mv > 59) {
                MessageBeep(MB_ICONASTERISK);
                SetFocus(GetDlgItem(dlg, IDC_ALM_MIN));
                return TRUE;
            }
            p->result.minute = (int)mv;

            // Date fields (only when Date schedule)
            if (p->result.schedule == AlarmSchedule::Date) {
                wchar_t ybuf[8] = {}, mobuf[4] = {}, dbuf[4] = {};
                GetDlgItemTextW(dlg, IDC_ALM_YEAR,  ybuf, 7);
                GetDlgItemTextW(dlg, IDC_ALM_MONTH, mobuf, 3);
                GetDlgItemTextW(dlg, IDC_ALM_DAY,   dbuf, 3);
                long yr = std::wcstol(ybuf, nullptr, 10);
                long mo = std::wcstol(mobuf, nullptr, 10);
                long dy = std::wcstol(dbuf, nullptr, 10);
                if (yr < 2000 || yr > 9999 || mo < 1 || mo > 12 || dy < 1 || dy > 31) {
                    MessageBeep(MB_ICONASTERISK);
                    SetFocus(GetDlgItem(dlg, IDC_ALM_YEAR));
                    return TRUE;
                }
                SYSTEMTIME st_check{(WORD)yr, (WORD)mo, 0, (WORD)dy, 0, 0, 0, 0};
                FILETIME ft_check;
                if (!SystemTimeToFileTime(&st_check, &ft_check)) {
                    MessageBeep(MB_ICONASTERISK);
                    SetFocus(GetDlgItem(dlg, IDC_ALM_DAY));
                    return TRUE;
                }
                p->result.date_year  = (int)yr;
                p->result.date_month = (int)mo;
                p->result.date_day   = (int)dy;
            }

            EndDialog(dlg, IDOK);
            return TRUE;
        }
        case IDC_ALM_CANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { EndDialog(dlg, IDCANCEL); return TRUE; }
        if (wp == VK_RETURN) { SendMessageW(dlg, WM_COMMAND, IDC_ALM_OK, 0); return TRUE; }
        break;
    }
    return FALSE;
}

// ─── Public API ───────────────────────────────────────────────────────────────

bool show_add_alarm_dialog(HWND parent, Alarm& out, HFONT font,
                                   const Theme* theme, int dpi) {
    if (!theme) theme = &dark_theme;
    if (dpi <= 0) dpi = STANDARD_DPI;

    // Default to today's date for Date schedule
    SYSTEMTIME st; GetLocalTime(&st);
    Alarm defaults;
    defaults.date_year  = st.wYear;
    defaults.date_month = st.wMonth;
    defaults.date_day   = st.wDay;

    Params p{
        .result = defaults,
        .style  = {.theme = theme, .font = font, .dpi = dpi},
        .brushes = {},
    };
    auto tmpl = build_template();
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    INT_PTR res = DialogBoxIndirectParamW(
        hInst,
        reinterpret_cast<DLGTEMPLATE*>(tmpl.data()),
        parent, DlgProc, (LPARAM)&p
    );

    if (res != IDOK) return false;
    out = p.result;
    out.enabled = true;
    return true;
}

} // namespace alarm_dlg
