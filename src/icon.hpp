#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include "gdi.hpp"

inline HICON create_app_icon(int size, bool dark = true) {
    HDC screen = GetDC(nullptr);
    if (!screen) return nullptr;

    DcObj mdc{CreateCompatibleDC(screen)};
    if (!mdc.h) { ReleaseDC(nullptr, screen); return nullptr; }

    GdiObj color{CreateCompatibleBitmap(screen, size, size)};
    GdiObj mask{CreateBitmap(size, size, 1, 1, nullptr)};
    if (!color.h || !mask.h) {
        ReleaseDC(nullptr, screen);
        return nullptr;
    }
    auto* old = SelectObject(mdc, color);

    GdiObj face{CreateSolidBrush(dark ? RGB(60, 60, 66) : RGB(230, 230, 235))};
    GdiObj outline{CreatePen(PS_SOLID, 1, dark ? RGB(100, 100, 110) : RGB(140, 140, 150))};
    GdiObj hand{CreatePen(PS_SOLID, size > 20 ? 2 : 1, dark ? RGB(204, 204, 204) : RGB(40, 40, 50))};

    RECT all{0, 0, size, size};
    FillRect(mdc, &all, (HBRUSH)GetStockObject(BLACK_BRUSH));

    SelectObject(mdc, face);
    SelectObject(mdc, outline);
    Ellipse(mdc, 1, 1, size - 1, size - 1);

    int cx = size / 2, cy = size / 2;
    int hr_len = size / 4;
    int mn_len = size / 3;
    SelectObject(mdc, hand);

    MoveToEx(mdc, cx, cy, nullptr);
    LineTo(mdc, cx - hr_len * 5 / 10, cy - hr_len * 9 / 10);

    MoveToEx(mdc, cx, cy, nullptr);
    LineTo(mdc, cx + mn_len * 5 / 10, cy - mn_len * 9 / 10);

    SelectObject(mdc, old);

    DcObj mdc2{CreateCompatibleDC(screen)};
    if (!mdc2.h) {
        ReleaseDC(nullptr, screen);
        return nullptr;
    }
    SelectObject(mdc2, mask);
    FillRect(mdc2, &all, (HBRUSH)GetStockObject(WHITE_BRUSH));
    SelectObject(mdc2, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SelectObject(mdc2, (HPEN)GetStockObject(NULL_PEN));
    Ellipse(mdc2, 1, 1, size - 1, size - 1);
    // mdc, mdc2, face, outline, hand cleaned up by RAII destructors

    ReleaseDC(nullptr, screen);

    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmMask = (HBITMAP)mask.h;
    ii.hbmColor = (HBITMAP)color.h;
    HICON icon = CreateIconIndirect(&ii);
    // color and mask cleaned up by GdiObj destructors
    return icon;
}
