// Included by ui_windows.hpp inside namespace settings_dlg.

static RECT map_dlu(HWND dlg, short x, short y, short cx, short cy) {
    RECT r = {x, y, x + cx, y + cy};
    MapDialogRect(dlg, &r);
    return r;
}

constexpr int ANALOG_COLOR_COUNT = 9;
constexpr int ANALOG_VALUE_COUNT = 15;

struct HitRects {
    RECT sidebar[TAB_COUNT];
    RECT preset_row[5];
    RECT analog_preview;
    RECT analog_min_ticks;
    RECT analog_labels[3];
    RECT analog_colors[ANALOG_COLOR_COUNT];
    RECT analog_values[ANALOG_VALUE_COUNT];
};

static HitRects compute_rects(HWND dlg) {
    HitRects h{};
    h.sidebar[0] = map_dlu(dlg, 3, 24, 56, 14);
    h.sidebar[1] = map_dlu(dlg, 3, 40, 56, 14);
    h.sidebar[2] = map_dlu(dlg, 3, 56, 56, 14);
    h.sidebar[3] = map_dlu(dlg, 3, 72, 56, 14);

    // Analog settings sit below the format dropdown (which is at y=40, h=12).
    // "Live preview" label at y=64 (h=8), preview fixed (sticky) at y=76; right column starts at x=158.
    h.analog_preview = map_dlu(dlg, 68, 76, 80, 80);
    h.analog_min_ticks = map_dlu(dlg, 158,  64, 122, 12);
    h.analog_labels[0] = map_dlu(dlg, 158,  92,  40, 12);
    h.analog_labels[1] = map_dlu(dlg, 202,  92,  40, 12);
    h.analog_labels[2] = map_dlu(dlg, 246,  92,  40, 12);
    for (int i = 0; i < ANALOG_COLOR_COUNT; ++i)
        h.analog_colors[i] = map_dlu(dlg, 158, (short)(122 + i * 14), 122, 12);
    for (int i = 0; i < ANALOG_VALUE_COUNT; ++i)
        h.analog_values[i] = map_dlu(dlg, 68, (short)(258 + i * 14), 214, 12);

    for (int i = 0; i < 5; ++i)
        h.preset_row[i] = map_dlu(dlg, 96, (short)(40 + i * 16), 50, 12);
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
    GdiObj brush_bg{};
    GdiObj brush_edit{};
    GdiObj brush_btn{};
    GdiObj brush_combo{};
};
