#pragma once
#include <vector>
#include <windows.h>
#include "dwm_fwd.hpp"
#include "gdi.hpp"
#include "layout.hpp"
#include "theme.hpp"

// ─── Runtime DLGTEMPLATE builder ─────────────────────────────────────────────

struct DlgBuf {
    std::vector<WORD> data;
    void align4()            { while (data.size() % 2) data.push_back(0); }
    void push_word(WORD w)   { data.push_back(w); }
    void push_dword(DWORD d) { data.push_back(LOWORD(d)); data.push_back(HIWORD(d)); }
    void push_wstr(const wchar_t* s) { while (*s) data.push_back((WORD)*s++); data.push_back(0); }
    void push_wstr_empty()   { data.push_back(0); }
};

void dlg_add_item(DlgBuf& b, DWORD style, short x, short y, short cx, short cy,
                  WORD id, WORD cls_atom, const wchar_t* title);

// ─── Reusable owner-drawn dialog styling ─────────────────────────────────────

struct DlgStyle {
    const Theme* theme = &dark_theme;
    HFONT font = nullptr;
    int dpi = STANDARD_DPI;

    int scale(int v) const { return v * dpi / STANDARD_DPI; }

    void apply_dark_mode(HWND hwnd) const;
    void fill_background(HDC hdc, const RECT& rc) const;
    void draw_label(HDC hdc, const RECT& rc, const wchar_t* text,
                    UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE) const;
    void draw_button(HDC hdc, const RECT& rc, const wchar_t* text, bool focused) const;
    void draw_check_radio(HDC hdc, const RECT& rc, const wchar_t* text,
                          bool checked, bool is_radio) const;
};
