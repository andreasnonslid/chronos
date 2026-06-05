#include "ui_render.hpp"

// ─── Settings modal ──────────────────────────────────────────────────────────

static void open_settings(App& app, UiState& ui) {
    ui.pending_theme      = app.theme_mode;
    ui.pending_clock_view = app.clock_view;
    ui.pending_clock_split_mode = app.clock_split_mode;
    ui.pending_clock_split_pct  = app.clock_split_pct;
    ui.pending_analog     = app.analog_style;
    ui.pending_sound      = app.sound_on_expiry;
    ui.pending_work_min   = app.pomodoro_work_secs / 60;
    ui.pending_short_min  = app.pomodoro_short_secs / 60;
    ui.pending_long_min   = app.pomodoro_long_secs / 60;
    ui.pending_cadence    = app.pomodoro_cadence;
    ui.pending_auto_start = app.pomodoro_auto_start;
    for (int i = 0; i < 5; ++i)
        ui.pending_presets[i] = i < (int)app.custom_preset_secs.size()
            ? app.custom_preset_secs[i] / 60 : 0;
}

void render_settings_window(App& app, UiState& ui) {
    if (!ui.show_settings) {
        ui.settings_initialized = false;
        return;
    }
    if (!ui.settings_initialized) {
        open_settings(app, ui);
        ui.settings_initialized = true;
    }

    ImGui::TextUnformatted("Settings");
    ImGui::Separator();

    if (ImGui::BeginTabBar("##tabs")) {
        // Use settings_tab as a one-shot pre-selection flag, then clear it
        int tab_to_select = ui.settings_tab;
        ui.settings_tab = -1;
        auto tab_flags = [tab_to_select](int idx) -> ImGuiTabItemFlags {
            return tab_to_select == idx ? ImGuiTabItemFlags_SetSelected : 0;
        };

        // ── Appearance ──
        if (ImGui::BeginTabItem("Appearance", nullptr, tab_flags(0))) {
            ImGui::Text("Theme");
            int tm = (int)ui.pending_theme;
            ImGui::RadioButton("Auto",  &tm, 0); CHRONOS_DEBUG_ITEM("theme auto", IM_COL32(80, 220, 255, 255)); ImGui::SameLine();
            ImGui::RadioButton("Light", &tm, 1); CHRONOS_DEBUG_ITEM("theme light", IM_COL32(80, 220, 255, 255)); ImGui::SameLine();
            ImGui::RadioButton("Dark",  &tm, 2); CHRONOS_DEBUG_ITEM("theme dark", IM_COL32(80, 220, 255, 255));
            ui.pending_theme = (ThemeMode)tm;

            ImGui::Spacing();
            ImGui::Text("Notifications");
            ImGui::Checkbox("Sound on expiry", &ui.pending_sound);
            CHRONOS_DEBUG_ITEM("sound expiry", IM_COL32(80, 220, 255, 255));
            ImGui::EndTabItem();
        }

        // ── Clock ──
        if (ImGui::BeginTabItem("Clock", nullptr, tab_flags(1))) {
            const char* view_names[] = {
                "24h + seconds","24h","12h + seconds","12h",
                "Analog","Analog + 24h","24h / 12h","Analog + 24h/12h"
            };
            int cv = (int)ui.pending_clock_view;
            ImGui::SetNextItemWidth(200.f);
            ImGui::Combo("Format", &cv, view_names, CLOCK_VIEW_COUNT);
            CHRONOS_DEBUG_ITEM("clock format", IM_COL32(80, 220, 255, 255));
            ui.pending_clock_view = (ClockView)cv;

            if (clock_view_is_mixed(ui.pending_clock_view)) {
                const char* split_names[] = {"Auto", "Horizontal", "Vertical"};
                int sm = (int)ui.pending_clock_split_mode;
                ImGui::SetNextItemWidth(160.f);
                ImGui::Combo("Split", &sm, split_names, CLOCK_SPLIT_MODE_COUNT);
                CHRONOS_DEBUG_ITEM("clock split mode", IM_COL32(80, 220, 255, 255));
                ui.pending_clock_split_mode = (ClockSplitMode)sm;
                ImGui::SetNextItemWidth(200.f);
                ImGui::SliderInt("First pane (%)", &ui.pending_clock_split_pct, 20, 80);
                CHRONOS_DEBUG_ITEM("clock split pct", IM_COL32(80, 220, 255, 255));
            }

            if (clock_view_has_analog(ui.pending_clock_view)) {
                ImGui::Separator();
                // Live preview
                float prev_size = 120.f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::Dummy({prev_size, prev_size});
                tm lt = local_tm(std::time(nullptr));
                draw_analog_clock_imgui(ImGui::GetWindowDrawList(),
                    pos.x + prev_size / 2.f, pos.y + prev_size / 2.f, prev_size / 2.f - 4.f,
                    ui.pending_analog, palette_for(ui.pending_theme, false),
                    lt.tm_hour, lt.tm_min, lt.tm_sec);

                ImGui::Separator();
                ImGui::Text("Hand lengths (%%)");
                ImGui::SliderInt("Hour len",   &ui.pending_analog.hour_len_pct,   0, 100);
                CHRONOS_DEBUG_ITEM("hour length", IM_COL32(80, 220, 255, 255));
                ImGui::SliderInt("Minute len", &ui.pending_analog.minute_len_pct, 0, 100);
                CHRONOS_DEBUG_ITEM("minute length", IM_COL32(80, 220, 255, 255));
                ImGui::SliderInt("Second len", &ui.pending_analog.second_len_pct, 0, 100);
                CHRONOS_DEBUG_ITEM("second length", IM_COL32(80, 220, 255, 255));
                ImGui::Text("Thickness");
                ImGui::SliderInt("Hour thick",   &ui.pending_analog.hour_thickness,   1, 6);
                CHRONOS_DEBUG_ITEM("hour thick", IM_COL32(80, 220, 255, 255));
                ImGui::SliderInt("Minute thick", &ui.pending_analog.minute_thickness, 1, 4);
                CHRONOS_DEBUG_ITEM("minute thick", IM_COL32(80, 220, 255, 255));
                ImGui::SliderInt("Second thick", &ui.pending_analog.second_thickness, 1, 3);
                CHRONOS_DEBUG_ITEM("second thick", IM_COL32(80, 220, 255, 255));
                ImGui::Checkbox("Minute ticks", &ui.pending_analog.show_minute_ticks);
                CHRONOS_DEBUG_ITEM("minute ticks", IM_COL32(80, 220, 255, 255));
                int hl = (int)ui.pending_analog.hour_labels;
                ImGui::RadioButton("No labels",    &hl, 0); CHRONOS_DEBUG_ITEM("no labels", IM_COL32(80, 220, 255, 255)); ImGui::SameLine();
                ImGui::RadioButton("Sparse",       &hl, 1); CHRONOS_DEBUG_ITEM("sparse labels", IM_COL32(80, 220, 255, 255)); ImGui::SameLine();
                ImGui::RadioButton("Full",         &hl, 2); CHRONOS_DEBUG_ITEM("full labels", IM_COL32(80, 220, 255, 255));
                ui.pending_analog.hour_labels = (HourLabels)hl;
            }
            ImGui::EndTabItem();
        }

        // ── Pomodoro ──
        if (ImGui::BeginTabItem("Pomodoro", nullptr, tab_flags(2))) {
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Work (min)",        &ui.pending_work_min,  1);
            CHRONOS_DEBUG_ITEM("work minutes", IM_COL32(80, 220, 255, 255));
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Short break (min)", &ui.pending_short_min, 1);
            CHRONOS_DEBUG_ITEM("short minutes", IM_COL32(80, 220, 255, 255));
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Long break (min)",  &ui.pending_long_min,  1);
            CHRONOS_DEBUG_ITEM("long minutes", IM_COL32(80, 220, 255, 255));
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Long every N sessions", &ui.pending_cadence, 1);
            CHRONOS_DEBUG_ITEM("long cadence", IM_COL32(80, 220, 255, 255));
            ui.pending_work_min  = std::clamp(ui.pending_work_min,  1, 120);
            ui.pending_short_min = std::clamp(ui.pending_short_min, 1, 60);
            ui.pending_long_min  = std::clamp(ui.pending_long_min,  1, 120);
            ui.pending_cadence   = std::clamp(ui.pending_cadence,
                                              POMODORO_MIN_CADENCE, POMODORO_MAX_CADENCE);
            ImGui::Checkbox("Auto-start next phase", &ui.pending_auto_start);
            CHRONOS_DEBUG_ITEM("auto start", IM_COL32(80, 220, 255, 255));
            ImGui::EndTabItem();
        }

        // ── Timers ──
        if (ImGui::BeginTabItem("Timers", nullptr, tab_flags(3))) {
            ImGui::Text("Custom presets");
            for (int i = 0; i < 5; ++i) {
                ImGui::SetNextItemWidth(80);
                ImGui::InputInt(std::format("Preset {} (min)", i + 1).c_str(),
                                &ui.pending_presets[i], 1);
                CHRONOS_DEBUG_ITEM("timer preset", IM_COL32(80, 220, 255, 255));
                ui.pending_presets[i] = std::clamp(ui.pending_presets[i], 0, 1440);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    if (ImGui::Button("Apply")) {
        app.theme_mode          = ui.pending_theme;
        app.clock_view          = ui.pending_clock_view;
        app.clock_split_mode    = ui.pending_clock_split_mode;
        app.clock_split_pct     = std::clamp(ui.pending_clock_split_pct, 20, 80);
        app.analog_style        = ui.pending_analog;
        app.sound_on_expiry     = ui.pending_sound;
        app.pomodoro_work_secs  = ui.pending_work_min  * 60;
        app.pomodoro_short_secs = ui.pending_short_min * 60;
        app.pomodoro_long_secs  = ui.pending_long_min  * 60;
        app.pomodoro_cadence    = ui.pending_cadence;
        app.pomodoro_auto_start = ui.pending_auto_start;
        app.custom_preset_secs.clear();
        for (int i = 0; i < 5; ++i)
            if (ui.pending_presets[i] > 0)
                app.custom_preset_secs.push_back(ui.pending_presets[i] * 60);
        apply_imgui_theme(app.theme_mode, false);
        ui.dirty = true;
        ui.show_settings = false;
        ui.settings_initialized = false;
    }
    CHRONOS_DEBUG_ITEM("settings apply", IM_COL32(80, 220, 255, 255));
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        ui.show_settings = false;
        ui.settings_initialized = false;
    }
    CHRONOS_DEBUG_ITEM("settings cancel", IM_COL32(80, 220, 255, 255));
}
