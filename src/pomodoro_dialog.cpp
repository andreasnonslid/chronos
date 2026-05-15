#include "pomodoro_dialog.hpp"
#include <windows.h>
#include <windowsx.h>
#include <format>
#include "app.hpp"
#include "dialog_style.hpp"

// ─── Dialog control IDs ───────────────────────────────────────────────────────

constexpr int IDC_POMO_WORK  = 101;
constexpr int IDC_POMO_SHORT = 102;
constexpr int IDC_POMO_LONG  = 103;
constexpr int IDC_BTN_OK     = 104;
constexpr int IDC_BTN_CANCEL = 105;

// ─── Runtime DLGTEMPLATE builder ─────────────────────────────────────────────

namespace pomo_dlg_detail {

constexpr WORD CLS_BUTTON = 0x0080;
constexpr WORD CLS_EDIT   = 0x0081;
constexpr WORD CLS_STATIC = 0x0082;

// 3 labels + 3 edits + 3 "min" statics + 2 buttons = 11
constexpr WORD DLG_ITEM_COUNT = 11;
constexpr short DLG_W = 170, DLG_H = 110;

std::vector<WORD> build_template() {
    DlgBuf b;

    b.push_dword(WS_POPUP | WS_BORDER);
    b.push_dword(0);
    b.push_word(DLG_ITEM_COUNT);
    b.push_word(0); b.push_word(0);
    b.push_word(DLG_W); b.push_word(DLG_H);
    b.push_wstr_empty(); // no menu
    b.push_wstr_empty(); // default class
    b.push_wstr_empty(); // no caption text (we draw our own)

    // Layout: centered content
    // Title area occupies y 0-20 (painted by us)
    // Row starts at y=24, rows spaced 18du apart
    // label x=14 w=52, edit x=70 w=28, "min" x=102 w=18
    short row_y0 = 28, row_sp = 18;
    short lbl_x = 14, lbl_w = 54;
    short ed_x = 72, ed_w = 28;
    short min_x = 104, min_w = 18;
    short row_h = 12;

    // Row 0 — Work
    dlg_add_item(b, SS_RIGHT | SS_CENTERIMAGE,
                 lbl_x, row_y0, lbl_w, row_h, 0xFFFF, CLS_STATIC, L"Work");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
                 ed_x, row_y0, ed_w, row_h, IDC_POMO_WORK, CLS_EDIT, L"");
    dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
                 min_x, row_y0, min_w, row_h, 0xFFFF, CLS_STATIC, L"min");

    // Row 1 — Short break
    short r1 = row_y0 + row_sp;
    dlg_add_item(b, SS_RIGHT | SS_CENTERIMAGE,
                 lbl_x, r1, lbl_w, row_h, 0xFFFF, CLS_STATIC, L"Short break");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
                 ed_x, r1, ed_w, row_h, IDC_POMO_SHORT, CLS_EDIT, L"");
    dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
                 min_x, r1, min_w, row_h, 0xFFFF, CLS_STATIC, L"min");

    // Row 2 — Long break
    short r2 = row_y0 + 2 * row_sp;
    dlg_add_item(b, SS_RIGHT | SS_CENTERIMAGE,
                 lbl_x, r2, lbl_w, row_h, 0xFFFF, CLS_STATIC, L"Long break");
    dlg_add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP,
                 ed_x, r2, ed_w, row_h, IDC_POMO_LONG, CLS_EDIT, L"");
    dlg_add_item(b, SS_LEFT | SS_CENTERIMAGE,
                 min_x, r2, min_w, row_h, 0xFFFF, CLS_STATIC, L"min");

    // Buttons — centered at bottom
    short btn_y = row_y0 + 3 * row_sp + 4;
    short btn_w = 44, btn_h = 16, btn_gap = 8;
    short total_w = 2 * btn_w + btn_gap;
    short btn_x0 = (DLG_W - total_w) / 2;
    dlg_add_item(b, BS_OWNERDRAW | WS_TABSTOP,
                 btn_x0, btn_y, btn_w, btn_h, IDC_BTN_OK, CLS_BUTTON, L"OK");
    dlg_add_item(b, BS_OWNERDRAW | WS_TABSTOP,
                 btn_x0 + btn_w + btn_gap, btn_y, btn_w, btn_h, IDC_BTN_CANCEL, CLS_BUTTON, L"Cancel");

    return b.data;
}

// ─── Dialog parameters ──────────────────────────────────────────────────────

struct Params {
    int work_min, short_min, long_min;
    DlgStyle style;
    DialogBrushes brushes{};
};

bool read_field(HWND dlg, int id, int& out) {
    wchar_t buf[8] = {};
    GetDlgItemTextW(dlg, id, buf, 7);
    wchar_t* end = nullptr;
    long v = std::wcstol(buf, &end, 10);
    if (!end || *end != L'\0' || v < 1 || v > 1440) return false;
    out = (int)v;
    return true;
}

INT_PTR CALLBACK PomoDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        auto* p = reinterpret_cast<Params*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)p);

        p->style.apply_dark_mode(dlg);
        dialog_center_on_parent(dlg);

        SetDlgItemTextW(dlg, IDC_POMO_WORK,  std::format(L"{}", p->work_min).c_str());
        SetDlgItemTextW(dlg, IDC_POMO_SHORT, std::format(L"{}", p->short_min).c_str());
        SetDlgItemTextW(dlg, IDC_POMO_LONG,  std::format(L"{}", p->long_min).c_str());
        SendDlgItemMessageW(dlg, IDC_POMO_WORK,  EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_POMO_SHORT, EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_POMO_LONG,  EM_SETLIMITTEXT, 4, 0);

        dialog_apply_font_to_children(dlg, p->style.font);
        p->brushes = DialogBrushes::create(*p->style.theme);

        SetFocus(GetDlgItem(dlg, IDC_POMO_WORK));
        SendDlgItemMessageW(dlg, IDC_POMO_WORK, EM_SETSEL, 0, -1);
        return FALSE;
    }
    case WM_ERASEBKGND: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        RECT rc;
        GetClientRect(dlg, &rc);
        p->style.fill_background((HDC)wp, rc);
        dialog_paint_title((HDC)wp, dlg, p->style, L"Pomodoro Intervals", 22);
        return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        return dialog_handle_ctl_color(msg, (HDC)wp, *p->style.theme, p->brushes);
    }
    case WM_NCHITTEST:
        if (auto r = dialog_handle_caption_hittest(dlg, lp, 22)) return r;
        break;
    case WM_DRAWITEM: {
        auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (!p) break;
        auto* di = (DRAWITEMSTRUCT*)lp;
        if (di->CtlType != ODT_BUTTON) break;
        const wchar_t* label = (di->CtlID == IDC_BTN_OK) ? L"OK" : L"Cancel";
        bool focused = (di->itemState & ODS_FOCUS) != 0;
        p->style.draw_button(di->hDC, di->rcItem, label, focused);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_OK: {
            if (HIWORD(wp) != BN_CLICKED) break;
            auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
            int w = 0, s = 0, l = 0;
            if (!read_field(dlg, IDC_POMO_WORK,  w)) { MessageBeep(MB_ICONASTERISK); SetFocus(GetDlgItem(dlg, IDC_POMO_WORK));  return TRUE; }
            if (!read_field(dlg, IDC_POMO_SHORT, s)) { MessageBeep(MB_ICONASTERISK); SetFocus(GetDlgItem(dlg, IDC_POMO_SHORT)); return TRUE; }
            if (!read_field(dlg, IDC_POMO_LONG,  l)) { MessageBeep(MB_ICONASTERISK); SetFocus(GetDlgItem(dlg, IDC_POMO_LONG));  return TRUE; }
            p->work_min = w; p->short_min = s; p->long_min = l;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        case IDC_BTN_CANCEL:
            if (HIWORD(wp) != BN_CLICKED) break;
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { EndDialog(dlg, IDCANCEL); return TRUE; }
        if (wp == VK_RETURN) { SendMessageW(dlg, WM_COMMAND, IDC_BTN_OK, 0); return TRUE; }
        break;
    }
    return FALSE;
}

} // namespace pomo_dlg_detail

// ─── Public API ──────────────────────────────────────────────────────────────

bool show_pomodoro_interval_dialog(HWND parent, App& app, HFONT font,
                                          const Theme* theme, int dpi) {
    using namespace pomo_dlg_detail;

    if (!theme) theme = &dark_theme;
    if (dpi <= 0) dpi = STANDARD_DPI;

    Params p{
        .work_min  = app.pomodoro_work_secs  / 60,
        .short_min = app.pomodoro_short_secs / 60,
        .long_min  = app.pomodoro_long_secs  / 60,
        .style = {.theme = theme, .font = font, .dpi = dpi},
    };

    auto tmpl = build_template();
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    INT_PTR result = DialogBoxIndirectParamW(
        hInst,
        reinterpret_cast<DLGTEMPLATE*>(tmpl.data()),
        parent, PomoDlgProc, (LPARAM)&p
    );

    if (result != IDOK) return false;
    app.pomodoro_work_secs  = p.work_min  * 60;
    app.pomodoro_short_secs = p.short_min * 60;
    app.pomodoro_long_secs  = p.long_min  * 60;
    return true;
}
