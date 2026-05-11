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
};
