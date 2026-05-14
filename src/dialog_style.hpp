#pragma once
#include <windows.h>
#include "dwm_fwd.hpp"
#include "gdi.hpp"
#include "layout.hpp"
#include "theme.hpp"

// Reusable owner-drawn dialog styling for Chronos dialogs.
// Ensures popups share the same visual language as the main window.

struct DlgStyle {
    const Theme* theme = &dark_theme;
    HFONT font = nullptr;
    int dpi = STANDARD_DPI;

    int scale(int v) const { return v * dpi / STANDARD_DPI; }

    void apply_dark_mode(HWND hwnd) const {
        bool dark = (theme == &dark_theme);
        BOOL val = dark ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR, &val, sizeof(val));
    }

    void fill_background(HDC hdc, const RECT& rc) const {
        HBRUSH br = CreateSolidBrush(theme->bg);
        FillRect(hdc, &rc, br);
        DeleteObject(br);
    }

    void draw_label(HDC hdc, const RECT& rc, const wchar_t* text, UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE) const {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme->text);
        SelectObject(hdc, font);
        RECT r = rc;
        DrawTextW(hdc, text, -1, &r, fmt);
    }

    void draw_button(HDC hdc, const RECT& rc, const wchar_t* text, bool focused) const {
        HBRUSH bg_br = CreateSolidBrush(theme->bg);
        FillRect(hdc, &rc, bg_br);
        DeleteObject(bg_br);
        int rr = scale(6);
        HBRUSH br = CreateSolidBrush(focused ? theme->active : theme->btn);
        HPEN pen = CreatePen(PS_NULL, 0, 0);
        auto* obr = (HBRUSH)SelectObject(hdc, br);
        auto* opn = (HPEN)SelectObject(hdc, pen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, rr, rr);
        SelectObject(hdc, obr);
        SelectObject(hdc, opn);
        DeleteObject(br);
        DeleteObject(pen);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme->text);
        SelectObject(hdc, font);
        RECT r = rc;
        DrawTextW(hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void draw_check_radio(HDC hdc, const RECT& rc, const wchar_t* text, bool checked, bool is_radio) const {
        HBRUSH bg_br = CreateSolidBrush(theme->bg);
        FillRect(hdc, &rc, bg_br);
        DeleteObject(bg_br);

        int h = rc.bottom - rc.top;
        int gs = scale(10);
        if (gs > h - 2) gs = h - 2;
        int gy = rc.top + (h - gs) / 2;
        RECT g = { rc.left + 2, gy, rc.left + 2 + gs, gy + gs };

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
        RECT tr = { rc.left + gs + 6, rc.top, rc.right, rc.bottom };
        DrawTextW(hdc, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
};
