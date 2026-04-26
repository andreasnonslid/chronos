#pragma once
#include <windows.h>
#include <format>
#include <vector>
#include "app.hpp"

// ─── Dialog control IDs ───────────────────────────────────────────────────────

constexpr int IDC_POMO_WORK  = 101;
constexpr int IDC_POMO_SHORT = 102;
constexpr int IDC_POMO_LONG  = 103;

// ─── Runtime DLGTEMPLATE builder ─────────────────────────────────────────────

namespace pomo_dlg_detail {

// Built-in window class atoms used in dialog item templates
constexpr WORD CLS_BUTTON = 0x0080;
constexpr WORD CLS_EDIT   = 0x0081;
constexpr WORD CLS_STATIC = 0x0082;

struct Buf {
    std::vector<WORD> data;

    // Pad to next DWORD boundary (required between DLGITEMTEMPLATE entries)
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
    b.push_word((WORD)x);  b.push_word((WORD)y);
    b.push_word((WORD)cx); b.push_word((WORD)cy);
    b.push_word(id);
    b.push_word(0xFFFF); b.push_word(cls_atom); // built-in class by atom
    b.push_wstr(title);
    b.push_word(0); // creation data size
}

// 3 rows × (label + edit + "min" static) + OK + Cancel = 11 items
constexpr WORD DLG_ITEM_COUNT = 11;

// Dialog dimensions in dialog units
constexpr short DLG_W = 160, DLG_H = 82;

inline std::vector<WORD> build_template() {
    Buf b;

    // DLGTEMPLATE
    b.push_dword(DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU);
    b.push_dword(0); // exStyle
    b.push_word(DLG_ITEM_COUNT);
    b.push_word(0); b.push_word(0);   // x, y
    b.push_word(DLG_W); b.push_word(DLG_H);
    b.push_wstr_empty(); // no menu
    b.push_wstr_empty(); // default window class
    b.push_wstr(L"Pomodoro Intervals");

    // Layout: label col x=8 w=52, edit col x=62 w=30, "min" col x=95 w=20
    // Rows at y = 14, 30, 46.  Row height = 12.
    // Buttons at y = 62.

    // Row 0 — Work
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             8, 14, 52, 12, 0xFFFF, CLS_STATIC, L"Work");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, WS_EX_CLIENTEDGE,
             62, 14, 30, 12, IDC_POMO_WORK, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             95, 14, 20, 12, 0xFFFF, CLS_STATIC, L"min");

    // Row 1 — Short break
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             8, 30, 52, 12, 0xFFFF, CLS_STATIC, L"Short break");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, WS_EX_CLIENTEDGE,
             62, 30, 30, 12, IDC_POMO_SHORT, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             95, 30, 20, 12, 0xFFFF, CLS_STATIC, L"min");

    // Row 2 — Long break
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             8, 46, 52, 12, 0xFFFF, CLS_STATIC, L"Long break");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, WS_EX_CLIENTEDGE,
             62, 46, 30, 12, IDC_POMO_LONG, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             95, 46, 20, 12, 0xFFFF, CLS_STATIC, L"min");

    // Buttons
    add_item(b, BS_DEFPUSHBUTTON | WS_TABSTOP, 0,
             34, 64, 40, 14, IDOK,     CLS_BUTTON, L"OK");
    add_item(b, BS_PUSHBUTTON    | WS_TABSTOP, 0,
             80, 64, 40, 14, IDCANCEL, CLS_BUTTON, L"Cancel");

    return b.data;
}

// ─── Dialog parameters and proc ──────────────────────────────────────────────

struct Params {
    int work_min, short_min, long_min;
    HFONT font;
};

inline bool read_field(HWND dlg, int id, int& out) {
    wchar_t buf[8] = {};
    GetDlgItemTextW(dlg, id, buf, 7);
    wchar_t* end = nullptr;
    long v = std::wcstol(buf, &end, 10);
    if (!end || *end != L'\0' || v < 1 || v > 1440) return false;
    out = (int)v;
    return true;
}

inline INT_PTR CALLBACK PomoDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        auto* p = reinterpret_cast<Params*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)p);
        SetDlgItemTextW(dlg, IDC_POMO_WORK,  std::format(L"{}", p->work_min).c_str());
        SetDlgItemTextW(dlg, IDC_POMO_SHORT, std::format(L"{}", p->short_min).c_str());
        SetDlgItemTextW(dlg, IDC_POMO_LONG,  std::format(L"{}", p->long_min).c_str());
        SendDlgItemMessageW(dlg, IDC_POMO_WORK,  EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_POMO_SHORT, EM_SETLIMITTEXT, 4, 0);
        SendDlgItemMessageW(dlg, IDC_POMO_LONG,  EM_SETLIMITTEXT, 4, 0);
        if (p->font) {
            // Set the app font on all child controls
            EnumChildWindows(dlg, [](HWND child, LPARAM font) -> BOOL {
                SendMessageW(child, WM_SETFONT, (WPARAM)(HFONT)font, FALSE);
                return TRUE;
            }, (LPARAM)p->font);
        }
        SetFocus(GetDlgItem(dlg, IDC_POMO_WORK));
        SendDlgItemMessageW(dlg, IDC_POMO_WORK, EM_SETSEL, 0, -1);
        return FALSE; // focus set manually
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK: {
            auto* p = reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
            int w = 0, s = 0, l = 0;
            if (!read_field(dlg, IDC_POMO_WORK,  w)) { MessageBeep(MB_ICONASTERISK); SetFocus(GetDlgItem(dlg, IDC_POMO_WORK));  return TRUE; }
            if (!read_field(dlg, IDC_POMO_SHORT, s)) { MessageBeep(MB_ICONASTERISK); SetFocus(GetDlgItem(dlg, IDC_POMO_SHORT)); return TRUE; }
            if (!read_field(dlg, IDC_POMO_LONG,  l)) { MessageBeep(MB_ICONASTERISK); SetFocus(GetDlgItem(dlg, IDC_POMO_LONG));  return TRUE; }
            p->work_min = w; p->short_min = s; p->long_min = l;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

} // namespace pomo_dlg_detail

// ─── Public API ──────────────────────────────────────────────────────────────

// Show a modal dialog to edit Pomodoro work/short/long intervals.
// Applies changes to app on OK and returns true; returns false on Cancel.
inline bool show_pomodoro_interval_dialog(HWND parent, App& app, HFONT font) {
    using namespace pomo_dlg_detail;

    Params p{
        .work_min  = app.pomodoro_work_secs  / 60,
        .short_min = app.pomodoro_short_secs / 60,
        .long_min  = app.pomodoro_long_secs  / 60,
        .font      = font,
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
