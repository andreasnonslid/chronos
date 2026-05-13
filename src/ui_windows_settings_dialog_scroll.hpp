// Included by ui_windows.hpp inside namespace settings_dlg.

static void set_pomo_visible(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    for (int id = IDC_POMO_WORK; id <= IDC_MIN_CADENCE; ++id) {
        HWND h = GetDlgItem(dlg, id);
        if (h) ShowWindow(h, cmd);
    }
}

static void set_presets_visible(HWND dlg, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    int ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2, IDC_PRESET_ED3, IDC_PRESET_ED4,
                 IDC_PRESET_MIN0, IDC_PRESET_MIN1, IDC_PRESET_MIN2, IDC_PRESET_MIN3, IDC_PRESET_MIN4};
    for (int id : ids) {
        HWND h = GetDlgItem(dlg, id);
        if (h) ShowWindow(h, cmd);
    }
}

static void set_clock_combo_visible(HWND dlg, bool show) {
    HWND h = GetDlgItem(dlg, IDC_CLOCK_COMBO);
    if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
}


static void position_scrollable_controls(HWND dlg, const Params& p) {
    if (p.active_tab != TAB_CLOCK) return;

    HWND combo = GetDlgItem(dlg, IDC_CLOCK_COMBO);
    if (!combo || !IsWindowVisible(combo)) return;

    SetWindowPos(combo, nullptr,
                 p.clock_combo_rc.left,
                 p.clock_combo_rc.top - p.scroll_y,
                 p.clock_combo_rc.right - p.clock_combo_rc.left,
                 p.clock_combo_rc.bottom - p.clock_combo_rc.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

static int rect_bottom_after_scroll(const RECT& r, const Params& p) {
    return r.bottom - p.scroll_y;
}

static int tab_painted_content_bottom(HWND dlg, const Params& p) {
    switch (p.active_tab) {
    case TAB_APPEARANCE:
        return std::max({
            rect_bottom_after_scroll(p.rects.theme[0], p),
            rect_bottom_after_scroll(p.rects.theme[1], p),
            rect_bottom_after_scroll(p.rects.theme[2], p),
            rect_bottom_after_scroll(p.rects.sound, p),
        });
    case TAB_CLOCK:
        if (p.clock_view != ClockView::Analog) {
            return map_dlu(dlg, 70, 28, 80, 12).bottom;
        }
        return std::max({
            rect_bottom_after_scroll(p.rects.analog_min_ticks, p),
            rect_bottom_after_scroll(p.rects.analog_labels[0], p),
            rect_bottom_after_scroll(p.rects.analog_labels[1], p),
            rect_bottom_after_scroll(p.rects.analog_labels[2], p),
        });
    case TAB_POMODORO:
        return rect_bottom_after_scroll(p.rects.auto_start, p);
    case TAB_TIMERS:
        return map_dlu(dlg, 76, 104, 16, 12).bottom;
    default:
        return 0;
    }
}

static bool control_counts_as_scroll_content(int id) {
    return id != IDC_SET_OK && id != IDC_SET_CANCEL;
}

static int visible_child_content_bottom(HWND dlg) {
    int content_end = 0;
    EnumChildWindows(dlg, [](HWND child, LPARAM lp) -> BOOL {
        if (!IsWindowVisible(child)) return TRUE;
        int id = GetDlgCtrlID(child);
        if (!control_counts_as_scroll_content(id)) return TRUE;

        RECT r{};
        GetWindowRect(child, &r);
        MapWindowPoints(HWND_DESKTOP, GetParent(child), reinterpret_cast<POINT*>(&r), 2);
        auto* end = reinterpret_cast<int*>(lp);
        *end = std::max(*end, r.bottom);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&content_end));
    return content_end;
}

// Recompute scrollbar range from the actual visible controls plus painted tab content.
static void update_content_scroll(HWND dlg, Params* p) {
    p->scroll_y = 0;
    position_scrollable_controls(dlg, *p);

    RECT div_top = {0, 0, 0, TITLE_H + 2};
    RECT div_bot = {0, 0, 0, 132};
    MapDialogRect(dlg, &div_top);
    MapDialogRect(dlg, &div_bot);

    int view_h = div_bot.bottom - div_top.bottom;
    int content_bottom = std::max(visible_child_content_bottom(dlg), tab_painted_content_bottom(dlg, *p));
    int content_h = std::max(0, content_bottom - div_top.bottom);
    int max_scroll = std::max(0, content_h - view_h);

    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = content_h;
    si.nPage  = (UINT)view_h;
    si.nPos   = 0;
    SetScrollInfo(dlg, SB_VERT, &si, TRUE);
    EnableScrollBar(dlg, SB_VERT, max_scroll > 0 ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
}

// Apply scroll delta and reposition the combobox if on the clock tab.
static void apply_scroll(HWND dlg, Params* p, int new_pos) {
    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_POS, 0, 0, 0, 0, 0};
    si.nPos = new_pos;
    SetScrollInfo(dlg, SB_VERT, &si, TRUE);
    GetScrollInfo(dlg, SB_VERT, &si);
    p->scroll_y = si.nPos;

    position_scrollable_controls(dlg, *p);
    InvalidateRect(dlg, nullptr, TRUE);
}

