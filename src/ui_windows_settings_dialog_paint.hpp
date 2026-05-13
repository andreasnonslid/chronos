// Included by ui_windows.hpp inside namespace settings_dlg.

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

