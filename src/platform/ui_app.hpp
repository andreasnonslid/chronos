#pragma once
#include "app.hpp"
#include "config_io.hpp"
#include <filesystem>
#include <string>

// Mutable UI-layer state that lives alongside App but isn't persisted.
struct UiState {
    bool show_settings   = false;
    bool show_add_alarm  = false;
    bool dirty           = false;   // config needs saving

    // Settings modal working copy
    int  settings_tab    = 0;       // 0=Appearance 1=Clock 2=Pomodoro 3=Timers
    ThemeMode pending_theme;
    ClockView pending_clock_view;
    AnalogClockStyle pending_analog;
    bool pending_sound;
    int  pending_work_min, pending_short_min, pending_long_min, pending_cadence;
    bool pending_auto_start;
    int  pending_presets[5]{};

    // Add-alarm modal working copy
    char alarm_name[128]{};
    int  alarm_hour = 8, alarm_minute = 0;
    bool alarm_days_mode = true;    // true=days-of-week, false=specific date
    bool alarm_days[7]{true,true,true,true,true,true,true}; // Mon..Sun
    int  alarm_year = 2026, alarm_month = 1, alarm_day = 1;

    // Screenshot mode: if non-empty, save a frame to this path and quit
    std::string screenshot_path;

    std::filesystem::path cfg_path;
};

// Called once per ImGui frame. Renders everything.
void render_app(App& app, UiState& ui);

// Initialise ImGui theme from the app's theme mode + system preference.
void apply_imgui_theme(ThemeMode mode, bool system_prefers_dark);
