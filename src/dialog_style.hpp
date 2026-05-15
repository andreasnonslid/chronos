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

// ─── Shared dialog helpers ──────────────────────────────────────────────────

// Brush set used by every owner-drawn dialog: bg = window background,
// edit = edit/listbox background, btn = button background.
struct DialogBrushes {
    GdiObj bg;
    GdiObj edit;
    GdiObj btn;
    static DialogBrushes create(const Theme& theme);
};

// Center `dlg` over its owner; no-op if there is no parent.
void dialog_center_on_parent(HWND dlg);

// Send WM_SETFONT to every child of `dlg`.
void dialog_apply_font_to_children(HWND dlg, HFONT font);

// Handle WM_CTLCOLOR{STATIC,EDIT,BTN,LISTBOX}. Returns the brush
// handle as INT_PTR for use as the dialog-proc return value, or 0 if `msg`
// is not one of the four.
INT_PTR dialog_handle_ctl_color(UINT msg, HDC hdc, const Theme& theme,
                                const DialogBrushes& brushes);

// Handle WM_NCHITTEST for a dialog with a painted caption: drag by the
// top `title_h_dlu` dialog units. Returns HTCAPTION inside the title strip,
// 0 (i.e. fall through) otherwise.
INT_PTR dialog_handle_caption_hittest(HWND dlg, LPARAM lp, short title_h_dlu);

// Inside WM_ERASEBKGND, paint `title` centred in the top `title_h_dlu`
// dialog units and a horizontal divider line at the bottom edge.
void dialog_paint_title(HDC hdc, HWND dlg, const DlgStyle& style,
                        const wchar_t* title, short title_h_dlu);
