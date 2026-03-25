module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
export module icon;
import gdi;

export HICON create_app_icon(int size) {
    HDC screen = GetDC(nullptr);
    HDC mdc = CreateCompatibleDC(screen);
    HBITMAP color = CreateCompatibleBitmap(screen, size, size);
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    auto* old = SelectObject(mdc, color);

    GdiObj face{CreateSolidBrush(RGB(60, 60, 66))};
    GdiObj outline{CreatePen(PS_SOLID, 1, RGB(100, 100, 110))};
    GdiObj hand{CreatePen(PS_SOLID, size > 20 ? 2 : 1, RGB(204, 204, 204))};

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

    HDC mdc2 = CreateCompatibleDC(screen);
    SelectObject(mdc2, mask);
    HBRUSH white = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(mdc2, &all, white);
    SelectObject(mdc2, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SelectObject(mdc2, (HPEN)GetStockObject(NULL_PEN));
    Ellipse(mdc2, 1, 1, size - 1, size - 1);
    DeleteDC(mdc2);

    // face, outline, hand are cleaned up automatically by GdiObj destructors
    DeleteDC(mdc);
    ReleaseDC(nullptr, screen);

    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmMask = mask;
    ii.hbmColor = color;
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(color);
    DeleteObject(mask);
    return icon;
}
