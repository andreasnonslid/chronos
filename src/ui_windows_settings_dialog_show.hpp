// Included by ui_windows.hpp after namespace settings_dlg.


inline bool show_settings_dialog(WindowHandle parent, App& app, FontHandle font,
                                 ThemeHandle theme = nullptr, int dpi = 0) {
    using namespace settings_dlg;
    if (!theme) theme = &dark_theme;
    if (dpi <= 0) dpi = STANDARD_DPI;

    Params p{
        .active_tab    = TAB_APPEARANCE,
        .theme_mode    = app.theme_mode,
        .clock_view    = app.clock_view,
        .analog_style  = app.analog_style,
        .sound_on_expiry = app.sound_on_expiry,
        .work_min  = app.pomodoro_work_secs / 60,
        .short_min = app.pomodoro_short_secs / 60,
        .long_min  = app.pomodoro_long_secs / 60,
        .cadence   = app.pomodoro_cadence,
        .auto_start = app.pomodoro_auto_start,
        .preset_min = {},
        .style = {.theme = theme, .font = font, .dpi = dpi},
    };
    for (int i = 0; i < (int)app.custom_preset_secs.size() && i < 5; ++i)
        p.preset_min[i] = app.custom_preset_secs[i] / 60;

    auto tmpl = build_template();
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    INT_PTR result = DialogBoxIndirectParamW(
        hInst, reinterpret_cast<DLGTEMPLATE*>(tmpl.data()),
        parent, DlgProc, (LPARAM)&p
    );

    if (result != IDOK) return false;
    app.theme_mode       = p.theme_mode;
    app.clock_view       = p.clock_view;
    app.analog_style     = p.analog_style;
    app.sound_on_expiry  = p.sound_on_expiry;
    app.pomodoro_work_secs  = p.work_min * 60;
    app.pomodoro_short_secs = p.short_min * 60;
    app.pomodoro_long_secs  = p.long_min * 60;
    app.pomodoro_cadence    = p.cadence;
    app.pomodoro_auto_start = p.auto_start;
    app.custom_preset_secs.clear();
    for (int i = 0; i < 5; ++i)
        if (p.preset_min[i] > 0)
            app.custom_preset_secs.push_back(p.preset_min[i] * 60);
    return true;
}
