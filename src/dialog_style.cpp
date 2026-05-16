#include "dialog_style.hpp"
#include "ui_windows_painter.hpp"
#include <windowsx.h>

void dlg_add_item(DlgBuf& b, DWORD style, short x, short y, short cx, short cy, WORD id, WORD cls_atom,
                  const wchar_t* title) {
    b.align4();
    b.push_dword(WS_CHILD | WS_VISIBLE | style);
    b.push_dword(0);
    b.push_word((WORD)x);
    b.push_word((WORD)y);
    b.push_word((WORD)cx);
    b.push_word((WORD)cy);
    b.push_word(id);
    b.push_word(0xFFFF);
    b.push_word(cls_atom);
    b.push_wstr(title);
    b.push_word(0);
}

void DlgStyle::apply_dark_mode(HWND hwnd) const {
    bool dark = (theme == &dark_theme);
    BOOL val = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR, &val, sizeof(val));
}

void DlgStyle::fill_background(HDC hdc, const RECT& rc) const {
    DialogPaint dialog = make_dialog(theme->palette, DialogConfig{});
    GdiObj br = win_brush(dialog.background);
    FillRect(hdc, &rc, (HBRUSH)br.h);
}

void DlgStyle::draw_label(HDC hdc, const RECT& rc, const wchar_t* text, UINT fmt) const {
    win_paint_label(hdc, rc, text, font, theme->palette, fmt);
}

void DlgStyle::draw_button(HDC hdc, const RECT& rc, const wchar_t* text, bool focused) const {
    GdiObj bg_br = win_brush(theme->palette.bg);
    FillRect(hdc, &rc, (HBRUSH)bg_br.h);
    WidgetPaint button = make_button(theme->palette, ButtonConfig{
                                                         .active = focused,
                                                         .focused = focused,
                                                         .radius_px = scale(6),
                                                     });
    win_paint_button(hdc, rc, text, font, button);
}

void DlgStyle::draw_check_radio(HDC hdc, const RECT& rc, const wchar_t* text, bool checked, bool is_radio) const {
    HBRUSH bg_br = CreateSolidBrush(theme->bg);
    FillRect(hdc, &rc, bg_br);
    DeleteObject(bg_br);

    int h = rc.bottom - rc.top;
    int gs = scale(10);
    if (gs > h - 2) gs = h - 2;
    int gy = rc.top + (h - gs) / 2;
    RECT g = {rc.left + 2, gy, rc.left + 2 + gs, gy + gs};

    HPEN pen = CreatePen(PS_SOLID, 1, theme->text);
    HBRUSH fill = CreateSolidBrush(theme->bar);
    auto* opn = (HPEN)SelectObject(hdc, pen);
    auto* obr = (HBRUSH)SelectObject(hdc, fill);

    if (is_radio) {
        Ellipse(hdc, g.left, g.top, g.right, g.bottom);
        if (checked) {
            int margin = gs / 3;
            HBRUSH dot = CreateSolidBrush(theme->text);
            SelectObject(hdc, dot);
            Ellipse(hdc, g.left + margin, g.top + margin, g.right - margin, g.bottom - margin);
            SelectObject(hdc, obr);
            DeleteObject(dot);
        }
    } else {
        Rectangle(hdc, g.left, g.top, g.right, g.bottom);
        if (checked) {
            int m = gs / 5;
            MoveToEx(hdc, g.left + m, g.top + gs / 2, nullptr);
            LineTo(hdc, g.left + gs * 2 / 5, g.bottom - m - 1);
            LineTo(hdc, g.right - m, g.top + m);
        }
    }

    SelectObject(hdc, opn);
    SelectObject(hdc, obr);
    DeleteObject(pen);
    DeleteObject(fill);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme->text);
    SelectObject(hdc, font);
    RECT tr = {rc.left + gs + 6, rc.top, rc.right, rc.bottom};
    DrawTextW(hdc, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

// ─── Shared dialog helpers ──────────────────────────────────────────────────

DialogBrushes DialogBrushes::create(const Theme& theme) {
    return DialogBrushes{
        .bg = GdiObj{CreateSolidBrush(theme.bg)},
        .edit = GdiObj{CreateSolidBrush(theme.bar)},
        .btn = GdiObj{CreateSolidBrush(theme.bg)},
    };
}

void dialog_center_on_parent(HWND dlg) {
    HWND parent = GetParent(dlg);
    if (!parent) return;
    RECT pr{}, dr{};
    GetWindowRect(parent, &pr);
    GetWindowRect(dlg, &dr);
    int dw = dr.right - dr.left, dh = dr.bottom - dr.top;
    int x = pr.left + (pr.right - pr.left - dw) / 2;
    int y = pr.top + (pr.bottom - pr.top - dh) / 2;
    SetWindowPos(dlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void dialog_apply_font_to_children(HWND dlg, HFONT font) {
    EnumChildWindows(
        dlg,
        [](HWND child, LPARAM param) -> BOOL {
            SendMessageW(child, WM_SETFONT, (WPARAM)param, FALSE);
            return TRUE;
        },
        (LPARAM)font);
}

INT_PTR dialog_handle_ctl_color(UINT msg, HDC hdc, const Theme& theme, const DialogBrushes& brushes) {
    switch (msg) {
    case WM_CTLCOLORSTATIC:
        SetTextColor(hdc, theme.text);
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)brushes.bg.h;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        SetTextColor(hdc, theme.text);
        SetBkColor(hdc, theme.bar);
        return (INT_PTR)brushes.edit.h;
    case WM_CTLCOLORBTN:
        SetTextColor(hdc, theme.text);
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)brushes.btn.h;
    }
    return 0;
}

INT_PTR dialog_handle_caption_hittest(HWND dlg, LPARAM lp, short title_h_dlu) {
    RECT title{0, 0, 0, title_h_dlu};
    MapDialogRect(dlg, &title);
    POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    ScreenToClient(dlg, &pt);
    if (pt.y >= 0 && pt.y < title.bottom) return HTCAPTION;
    return 0;
}

void dialog_paint_title(HDC hdc, HWND dlg, const DlgStyle& style, const wchar_t* title, short title_h_dlu) {
    RECT client{};
    GetClientRect(dlg, &client);
    RECT title_rc{0, 0, 0, title_h_dlu};
    MapDialogRect(dlg, &title_rc);
    int divider_y = title_rc.bottom;
    title_rc.right = client.right;
    style.draw_label(hdc, title_rc, title, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DialogPaint dialog = make_dialog(style.theme->palette, DialogConfig{});
    GdiObj pen = win_pen(dialog.divider);
    auto* old = (HPEN)SelectObject(hdc, pen.h);
    MoveToEx(hdc, 0, divider_y, nullptr);
    LineTo(hdc, client.right, divider_y);
    SelectObject(hdc, old);
}
