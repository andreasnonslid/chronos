// Included by ui_windows.hpp inside namespace settings_dlg.

static RECT map_dlu(HWND dlg, short x, short y, short cx, short cy) {
    RECT r = {x, y, x + cx, y + cy};
    MapDialogRect(dlg, &r);
    return r;
}

struct HitRects {
    RECT sidebar[TAB_COUNT];
    RECT theme[3];
    RECT sound;
    RECT auto_start;
    RECT preset_row[5];
    RECT analog_min_ticks;
    RECT analog_labels[3];
};

static HitRects compute_rects(HWND dlg) {
    HitRects h{};
    h.sidebar[0] = map_dlu(dlg, 3, 24, 56, 14);
    h.sidebar[1] = map_dlu(dlg, 3, 40, 56, 14);
    h.sidebar[2] = map_dlu(dlg, 3, 56, 56, 14);
    h.sidebar[3] = map_dlu(dlg, 3, 72, 56, 14);

    h.theme[0] = map_dlu(dlg, 70, 42, 54, 13);
    h.theme[1] = map_dlu(dlg, 70, 57, 54, 13);
    h.theme[2] = map_dlu(dlg, 70, 72, 54, 13);

    // Analog settings sit below the format dropdown (which is at y=40, h=12)
    h.analog_min_ticks = map_dlu(dlg, 70,  72, 80, 12);
    h.analog_labels[0] = map_dlu(dlg, 70,  96, 34, 12);
    h.analog_labels[1] = map_dlu(dlg, 106, 96, 34, 12);
    h.analog_labels[2] = map_dlu(dlg, 142, 96, 34, 12);

    h.sound      = map_dlu(dlg, 70,  97, 100, 13);
    h.auto_start = map_dlu(dlg, 70, 115, 100, 13);
    return h;
}

struct Params {
    int active_tab = TAB_APPEARANCE;
    ThemeMode theme_mode;
    ClockView clock_view;
    AnalogClockStyle analog_style;
    bool sound_on_expiry;
    int work_min, short_min, long_min;
    int cadence;
    bool auto_start;
    int preset_min[5]{};
    DlgStyle style;
    HitRects rects{};
    int scroll_y = 0;       // current scroll offset in pixels
    RECT clock_combo_rc{};  // base pixel rect of the combobox (unscrolled)
};
