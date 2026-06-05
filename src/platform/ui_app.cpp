#include "ui_app.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include <chrono>
#include <cstring>
#include <format>
#include <string>
#include <imgui.h>
#include "actions.hpp"
#include "alarm.hpp"
#include "analog_clock_imgui.hpp"
#include "app.hpp"
#include "config.hpp"
#include "encoding.hpp"
#include "formatting.hpp"
#include "pomodoro.hpp"
#include "ui_style.hpp"

using namespace std::chrono;

// ─── Color helpers ────────────────────────────────────────────────────────────

static ImVec4 to_v4(UiColor c, float a = 1.f) {
    return {c.r / 255.f, c.g / 255.f, c.b / 255.f, a};
}

static ImU32 to_u32(UiColor c, float a = 1.f) {
    return IM_COL32(c.r, c.g, c.b, (int)(a * 255));
}

// ─── Theme ───────────────────────────────────────────────────────────────────

void apply_imgui_theme(ThemeMode mode, bool system_prefers_dark) {
    const auto& pal = palette_for(mode, system_prefers_dark);
    auto& s = ImGui::GetStyle();
    s.WindowRounding    = 4.f;
    s.FrameRounding     = 4.f;
    s.GrabRounding      = 3.f;
    s.PopupRounding     = 4.f;
    s.ScrollbarRounding = 4.f;
    s.WindowBorderSize  = 0.f;
    s.FrameBorderSize   = 0.f;
    s.ItemSpacing       = {6.f, 4.f};
    s.FramePadding      = {6.f, 3.f};

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]          = to_v4(pal.bg);
    c[ImGuiCol_ChildBg]           = to_v4(pal.bg);
    c[ImGuiCol_PopupBg]           = to_v4(pal.bar);
    c[ImGuiCol_FrameBg]           = to_v4(pal.btn);
    c[ImGuiCol_FrameBgHovered]    = to_v4(pal.active, 0.7f);
    c[ImGuiCol_FrameBgActive]     = to_v4(pal.active);
    c[ImGuiCol_TitleBg]           = to_v4(pal.bar);
    c[ImGuiCol_TitleBgActive]     = to_v4(pal.bar);
    c[ImGuiCol_MenuBarBg]         = to_v4(pal.bar);
    c[ImGuiCol_ScrollbarBg]       = to_v4(pal.bg);
    c[ImGuiCol_ScrollbarGrab]     = to_v4(pal.btn);
    c[ImGuiCol_ScrollbarGrabHovered] = to_v4(pal.active);
    c[ImGuiCol_ScrollbarGrabActive]  = to_v4(pal.active);
    c[ImGuiCol_CheckMark]         = to_v4(pal.text);
    c[ImGuiCol_SliderGrab]        = to_v4(pal.active);
    c[ImGuiCol_SliderGrabActive]  = to_v4(pal.text);
    c[ImGuiCol_Button]            = to_v4(pal.btn);
    c[ImGuiCol_ButtonHovered]     = to_v4(pal.active);
    c[ImGuiCol_ButtonActive]      = to_v4(pal.blink);
    c[ImGuiCol_Header]            = to_v4(pal.active, 0.6f);
    c[ImGuiCol_HeaderHovered]     = to_v4(pal.active, 0.8f);
    c[ImGuiCol_HeaderActive]      = to_v4(pal.active);
    c[ImGuiCol_Separator]         = to_v4(pal.divider);
    c[ImGuiCol_Text]              = to_v4(pal.text);
    c[ImGuiCol_TextDisabled]      = to_v4(pal.dim);
    c[ImGuiCol_Border]            = to_v4(pal.divider);
    c[ImGuiCol_Tab]               = to_v4(pal.btn);
    c[ImGuiCol_TabHovered]        = to_v4(pal.active);
    c[ImGuiCol_TabSelected]       = to_v4(pal.active);
    c[ImGuiCol_TabSelectedOverline] = to_v4(pal.active);
}

// ─── String helpers ───────────────────────────────────────────────────────────

static std::string ws(const std::wstring& w) { return wide_to_utf8(w); }

// ─── Toolbar ─────────────────────────────────────────────────────────────────

static void render_toolbar(App& app, UiState& ui, const ThemePalette& pal [[maybe_unused]]) {
    auto& s = ImGui::GetStyle();
    float bar_h = ImGui::GetFrameHeightWithSpacing() + s.ItemSpacing.y;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_v4(pal.bar));
    ImGui::BeginChild("##toolbar", {0, bar_h}, ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    auto toggle_btn = [&](const char* label, bool active, int action) {
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, to_v4(pal.active));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_v4(pal.active));
        }
        if (ImGui::Button(label)) {
            auto r = dispatch_action(app, action, steady_clock::now(), {});
            if (r.save_config) ui.dirty = true;
            if (r.apply_theme) apply_imgui_theme(app.theme_mode, false);
            if (r.open_settings) { ui.show_settings = true; ui.settings_tab = 0; }
        }
        if (active) ImGui::PopStyleColor(2);
        ImGui::SameLine();
    };

    toggle_btn("Pin",       app.topmost,    A_TOPMOST);
    toggle_btn("Clock",     app.show_clk,   A_SHOW_CLK);
    toggle_btn("Stopwatch", app.show_sw,    A_SHOW_SW);
    toggle_btn("Timers",    app.show_tmr,   A_SHOW_TMR);
    toggle_btn("Alarms",    app.show_alarms,A_SHOW_ALARMS);
    if (ImGui::Button("\xe2\x9a\x99")) {  // UTF-8 gear ⚙
        ui.show_settings = true;
        ui.settings_tab = 0;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Clock ───────────────────────────────────────────────────────────────────

static void render_clock(App& app, UiState& ui, const ThemePalette& pal) {
    if (!app.show_clk) return;

    auto now = steady_clock::now();
    time_t t = std::time(nullptr);
    tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    int h = lt.tm_hour, m = lt.tm_min, s = lt.tm_sec;

    bool has_analog = clock_view_has_analog(app.clock_view);

    if (has_analog) {
        float avail = ImGui::GetContentRegionAvail().x;
        float size  = std::min(avail, 180.f);
        ImVec2 pos  = ImGui::GetCursorScreenPos();
        float cx = pos.x + size / 2.f;
        float cy = pos.y + size / 2.f;
        float rad = size / 2.f - 6.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        draw_analog_clock_imgui(dl, cx, cy, rad, app.analog_style, pal, h, m, s);
        // Invisible button to capture the clock area for click-to-cycle
        ImGui::InvisibleButton("##clock_face", {size, size});
        if (ImGui::IsItemClicked()) {
            dispatch_action(app, A_CLK_CYCLE, now, {});
            ui.dirty = true;
        }

        if (clock_view_is_mixed(app.clock_view)) {
            ImGui::SameLine();
            ImGui::SetWindowFontScale(1.6f);
            // Mixed_AnalogIntlLocal shows 24h/12h stacked; others just 24h
            if (app.clock_view == ClockView::Mixed_AnalogIntlLocal) {
                int h12 = h % 12; if (h12 == 0) h12 = 12;
                ImGui::Text("%02d:%02d:%02d\n%d:%02d:%02d %s",
                    h, m, s, h12, m, s, h < 12 ? "AM" : "PM");
            } else {
                std::string txt = format_clock_text(app.clock_view, h, m, s);
                ImGui::TextUnformatted(txt.c_str());
            }
            ImGui::SetWindowFontScale(1.f);
        }
    } else if (app.clock_view == ClockView::Mixed_IntlLocal) {
        // 24h on top, 12h below — both lines clickable to cycle
        int h12 = h % 12; if (h12 == 0) h12 = 12;
        std::string txt24 = std::format("{:02}:{:02}:{:02}", h, m, s);
        std::string txt12 = std::format("{}:{:02}:{:02} {}", h12, m, s, h < 12 ? "AM" : "PM");
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetWindowFontScale(1.8f);
        float tw24 = ImGui::CalcTextSize(txt24.c_str()).x;
        if (tw24 < avail) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw24) / 2.f);
        if (ImGui::Selectable(txt24.c_str(), false, 0, {tw24, 0}))
            dispatch_action(app, A_CLK_CYCLE, now, {});
        float tw12 = ImGui::CalcTextSize(txt12.c_str()).x;
        if (tw12 < avail) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw12) / 2.f);
        if (ImGui::Selectable(txt12.c_str(), false, 0, {tw12, 0}))
            dispatch_action(app, A_CLK_CYCLE, now, {});
        ImGui::SetWindowFontScale(1.f);
    } else {
        std::string txt = format_clock_text(app.clock_view, h, m, s);
        float scale = 2.2f;
        ImGui::SetWindowFontScale(scale);
        float tw = ImGui::CalcTextSize(txt.c_str()).x;
        float avail = ImGui::GetContentRegionAvail().x;
        if (tw < avail) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw) / 2.f);
        if (ImGui::Selectable(txt.c_str(), false, 0, {avail, 0}))
            dispatch_action(app, A_CLK_CYCLE, now, {});
        ImGui::SetWindowFontScale(1.f);
    }
    ImGui::Separator();
}

// ─── Stopwatch ───────────────────────────────────────────────────────────────

static void render_stopwatch(App& app, [[maybe_unused]] UiState& ui, [[maybe_unused]] const ThemePalette& pal) {
    if (!app.show_sw) return;
    auto now = steady_clock::now();
    std::string elapsed = ws(format_stopwatch_display(app.sw.elapsed(now)));

    ImGui::SetWindowFontScale(1.5f);
    float tw = ImGui::CalcTextSize(elapsed.c_str()).x;
    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw) / 2.f);
    ImGui::TextUnformatted(elapsed.c_str());
    ImGui::SetWindowFontScale(1.f);

    auto btn = [&](const char* label, int action) {
        if (ImGui::Button(label)) {
            auto r = dispatch_action(app, action, now, {});
            if (r.save_config) ui.dirty = true;
        }
        ImGui::SameLine();
    };

    btn(app.sw.is_running() ? "Stop" : "Start", A_SW_START);
    btn("Lap",   A_SW_LAP);
    btn("Reset", A_SW_RESET);
    ImGui::NewLine();
    ImGui::Separator();
}

// ─── Timers ──────────────────────────────────────────────────────────────────

static void render_timers(App& app, [[maybe_unused]] UiState& ui, const ThemePalette& pal) {
    if (!app.show_tmr) return;
    auto now = steady_clock::now();

    for (int i = 0; i < (int)app.timers.size(); ++i) {
        auto& ts = app.timers[i];
        bool running = ts.t.is_running();
        bool expired = ts.t.touched() && ts.t.expired(now);
        bool untouched = !ts.t.touched();

        if (expired && !ts.notified) {
            ts.notified = true;
            if (app.sound_on_expiry) {
#ifdef _WIN32
                MessageBeep(MB_ICONASTERISK);
#else
                fputs("\a", stderr);
#endif
            }
            if (ts.pomodoro) {
                advance_pomodoro_phase(ts, app.pomodoro_work_secs, app.pomodoro_short_secs,
                                       app.pomodoro_long_secs, app.pomodoro_cadence,
                                       app.pomodoro_auto_start, now);
                ui.dirty = true;
            }
        }

        ImGui::PushID(i);

        // Label / pomodoro phase
        std::string lbl = ts.label.empty() ? std::format("Timer {}", i + 1) : ws(ts.label);
        ImGui::TextUnformatted(lbl.c_str());
        ImGui::SameLine();

        // Time display
        std::string tstr = running || ts.t.touched()
            ? ws(format_timer_display(ts.t.remaining(now)))
            : ws(format_timer_edit(ts.dur));

        if (expired)
            ImGui::PushStyleColor(ImGuiCol_Text, to_v4(pal.expire));
        ImGui::Text("%s", tstr.c_str());
        if (expired)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        // Start/Stop
        if (ImGui::SmallButton(running ? "Stop" : "Start"))
            dispatch_action(app, A_TMR_BASE + i * TMR_STRIDE + A_TMR_START, now, {});
        ImGui::SameLine();

        // Reset
        if (ImGui::SmallButton("Reset"))
            dispatch_action(app, A_TMR_BASE + i * TMR_STRIDE + A_TMR_RST, now, {});
        ImGui::SameLine();

        // Pomodoro toggle (only when untouched)
        if (untouched) {
            if (ts.pomodoro) ImGui::PushStyleColor(ImGuiCol_Button, to_v4(pal.active));
            if (ImGui::SmallButton("Pomo"))
                dispatch_action(app, A_TMR_BASE + i * TMR_STRIDE + A_TMR_POMO, now, {});
            if (ts.pomodoro) ImGui::PopStyleColor();
            ImGui::SameLine();
        }

        // Pomodoro skip (when running pomodoro)
        if (ts.pomodoro && ts.t.touched()) {
            if (ImGui::SmallButton("Skip"))
                dispatch_action(app, A_TMR_BASE + i * TMR_STRIDE + A_TMR_SKIP, now, {});
            ImGui::SameLine();
        }

        // Remove timer
        if ((int)app.timers.size() > 1 && ImGui::SmallButton("-")) {
            dispatch_action(app, A_TMR_BASE + i * TMR_STRIDE + A_TMR_DEL, now, {});
            ImGui::PopID();
            break;
        }

        // Duration editing row (when untouched and not pomodoro)
        if (untouched && !ts.pomodoro) {
            auto adj_btn = [&](const char* label, int off) {
                if (ImGui::SmallButton(label)) {
                    auto r = dispatch_action(app, A_TMR_BASE + i * TMR_STRIDE + off, now, {});
                    if (r.save_config) ui.dirty = true;
                }
            };
            ImGui::SetWindowFontScale(0.85f);
            adj_btn("-H", A_TMR_HDN); ImGui::SameLine();
            ImGui::TextUnformatted("H"); ImGui::SameLine();
            adj_btn("+H", A_TMR_HUP); ImGui::SameLine();
            ImGui::Spacing(); ImGui::SameLine();
            adj_btn("-M", A_TMR_MDN); ImGui::SameLine();
            ImGui::TextUnformatted("M"); ImGui::SameLine();
            adj_btn("+M", A_TMR_MUP); ImGui::SameLine();
            ImGui::Spacing(); ImGui::SameLine();
            adj_btn("-S", A_TMR_SDN); ImGui::SameLine();
            ImGui::TextUnformatted("S"); ImGui::SameLine();
            adj_btn("+S", A_TMR_SUP);
            ImGui::SetWindowFontScale(1.f);
        }

        // Progress bar for running/touched timers
        if (ts.t.touched()) {
            float dur_ms = (float)duration_cast<milliseconds>(ts.dur).count();
            float rem_ms = (float)duration_cast<milliseconds>(ts.t.remaining(now)).count();
            float frac   = dur_ms > 0 ? std::clamp(rem_ms / dur_ms, 0.f, 1.f) : 0.f;
            ImU32 fill_col = expired ? to_u32(pal.expire, 0.4f) : to_u32(pal.fill, 0.6f);
            ImVec2 p = ImGui::GetCursorScreenPos();
            float w = ImGui::GetContentRegionAvail().x;
            float h = 4.f;
            ImGui::GetWindowDrawList()->AddRectFilled(p, {p.x + w * frac, p.y + h}, fill_col);
            ImGui::Dummy({0, h});
        }

        ImGui::PopID();
    }

    // Add timer button
    if ((int)app.timers.size() < Config::MAX_TIMERS) {
        if (ImGui::SmallButton("+ Timer")) {
            int last = (int)app.timers.size() - 1;
            dispatch_action(app, A_TMR_BASE + last * TMR_STRIDE + A_TMR_ADD, steady_clock::now(), {});
        }
    }
    ImGui::Separator();
}

// ─── Alarms ──────────────────────────────────────────────────────────────────

static void render_alarms(App& app, UiState& ui) {
    if (!app.show_alarms) return;
    ImGui::Text("Alarms");
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Add") && (int)app.alarms.size() < ALARM_MAX_COUNT) {
        // Reset working copy
        memset(ui.alarm_name, 0, sizeof(ui.alarm_name));
        ui.alarm_hour = 8; ui.alarm_minute = 0;
        ui.alarm_days_mode = true;
        for (int d = 0; d < 7; ++d) ui.alarm_days[d] = true;
        {
            time_t t = std::time(nullptr); tm lt{};
#ifdef _WIN32
            localtime_s(&lt, &t);
#else
            localtime_r(&t, &lt);
#endif
            ui.alarm_year = lt.tm_year + 1900;
            ui.alarm_month = lt.tm_mon + 1;
            ui.alarm_day = lt.tm_mday;
        }
        ui.show_add_alarm = true;
    }

    if (app.alarms.empty()) {
        ImGui::TextDisabled("No alarms set");
    } else {
        for (int i = 0; i < (int)app.alarms.size(); ++i) {
            auto& a = app.alarms[i];
            ImGui::PushID(i);
            // Enabled toggle
            bool en = a.enabled;
            if (ImGui::Checkbox("##en", &en)) {
                dispatch_action(app, A_ALARM_TOGGLE + i, steady_clock::now(), {});
                ui.dirty = true;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(a.name.empty() ? "(unnamed)" : a.name.c_str());
            ImGui::SameLine();
            ImGui::Text("%02d:%02d", a.hour, a.minute);
            ImGui::SameLine();
            if (ImGui::SmallButton("Del")) {
                dispatch_action(app, A_ALARM_DEL + i, steady_clock::now(), {});
                ui.dirty = true;
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }
    ImGui::Separator();
}

// ─── Add-alarm modal ─────────────────────────────────────────────────────────

static void render_add_alarm_modal([[maybe_unused]] App& app, UiState& ui) {
    if (!ui.show_add_alarm) return;
    ImGui::OpenPopup("Add Alarm");
    ui.show_add_alarm = false;
}

static void render_add_alarm_popup([[maybe_unused]] App& app, UiState& ui) {
    if (!ImGui::BeginPopupModal("Add Alarm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::InputText("Name", ui.alarm_name, sizeof(ui.alarm_name));

    ImGui::SetNextItemWidth(50);
    ImGui::InputInt("Hour",   &ui.alarm_hour,   1);
    ui.alarm_hour   = std::clamp(ui.alarm_hour,   0, 23);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputInt("Minute", &ui.alarm_minute, 1);
    ui.alarm_minute = std::clamp(ui.alarm_minute, 0, 59);

    {
        int mode = ui.alarm_days_mode ? 1 : 0;
        ImGui::RadioButton("Days of week", &mode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Specific date", &mode, 0);
        ui.alarm_days_mode = (mode != 0);
    }

    if (ui.alarm_days_mode) {
        const char* day_names[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
        for (int d = 0; d < 7; ++d) {
            ImGui::Checkbox(day_names[d], &ui.alarm_days[d]);
            if (d < 6) ImGui::SameLine();
        }
    } else {
        ImGui::SetNextItemWidth(70); ImGui::InputInt("Year",  &ui.alarm_year,  1);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50); ImGui::InputInt("Month", &ui.alarm_month, 1);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50); ImGui::InputInt("Day",   &ui.alarm_day,   1);
        ui.alarm_month = std::clamp(ui.alarm_month, 1, 12);
        ui.alarm_day   = std::clamp(ui.alarm_day,   1, 31);
    }

    if (ImGui::Button("OK")) {
        Alarm a;
        a.name   = ui.alarm_name;
        a.hour   = ui.alarm_hour;
        a.minute = ui.alarm_minute;
        a.enabled = true;
        if (ui.alarm_days_mode) {
            a.schedule = AlarmSchedule::Days;
            a.days_mask = 0;
            for (int d = 0; d < 7; ++d)
                if (ui.alarm_days[d]) a.days_mask |= (1 << d);
        } else {
            a.schedule = AlarmSchedule::Date;
            a.date_year = ui.alarm_year;
            a.date_month = ui.alarm_month;
            a.date_day = ui.alarm_day;
        }
        app.alarms.push_back(a);
        ui.dirty = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

// ─── Settings modal ──────────────────────────────────────────────────────────

static void open_settings(App& app, UiState& ui) {
    ui.pending_theme      = app.theme_mode;
    ui.pending_clock_view = app.clock_view;
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

static void render_settings_modal(App& app, UiState& ui) {
    if (!ui.show_settings) return;
    open_settings(app, ui);
    ImGui::OpenPopup("Settings");
    ui.show_settings = false;
}

static void render_settings_popup(App& app, UiState& ui) {
    if (!ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

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
            ImGui::RadioButton("Auto",  &tm, 0); ImGui::SameLine();
            ImGui::RadioButton("Light", &tm, 1); ImGui::SameLine();
            ImGui::RadioButton("Dark",  &tm, 2);
            ui.pending_theme = (ThemeMode)tm;

            ImGui::Spacing();
            ImGui::Text("Notifications");
            ImGui::Checkbox("Sound on expiry", &ui.pending_sound);
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
            ui.pending_clock_view = (ClockView)cv;

            if (clock_view_has_analog(ui.pending_clock_view)) {
                ImGui::Separator();
                // Live preview
                float prev_size = 120.f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::Dummy({prev_size, prev_size});
                time_t t = std::time(nullptr);
                tm lt{};
#ifdef _WIN32
                localtime_s(&lt, &t);
#else
                localtime_r(&t, &lt);
#endif
                draw_analog_clock_imgui(ImGui::GetWindowDrawList(),
                    pos.x + prev_size / 2.f, pos.y + prev_size / 2.f, prev_size / 2.f - 4.f,
                    ui.pending_analog, palette_for(ui.pending_theme, false),
                    lt.tm_hour, lt.tm_min, lt.tm_sec);

                ImGui::Separator();
                ImGui::Text("Hand lengths (%%)");
                ImGui::SliderInt("Hour len",   &ui.pending_analog.hour_len_pct,   0, 100);
                ImGui::SliderInt("Minute len", &ui.pending_analog.minute_len_pct, 0, 100);
                ImGui::SliderInt("Second len", &ui.pending_analog.second_len_pct, 0, 100);
                ImGui::Text("Thickness");
                ImGui::SliderInt("Hour thick",   &ui.pending_analog.hour_thickness,   1, 6);
                ImGui::SliderInt("Minute thick", &ui.pending_analog.minute_thickness, 1, 4);
                ImGui::SliderInt("Second thick", &ui.pending_analog.second_thickness, 1, 3);
                ImGui::Checkbox("Minute ticks", &ui.pending_analog.show_minute_ticks);
                int hl = (int)ui.pending_analog.hour_labels;
                ImGui::RadioButton("No labels",    &hl, 0); ImGui::SameLine();
                ImGui::RadioButton("Sparse",       &hl, 1); ImGui::SameLine();
                ImGui::RadioButton("Full",         &hl, 2);
                ui.pending_analog.hour_labels = (HourLabels)hl;
            }
            ImGui::EndTabItem();
        }

        // ── Pomodoro ──
        if (ImGui::BeginTabItem("Pomodoro", nullptr, tab_flags(2))) {
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Work (min)",        &ui.pending_work_min,  1);
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Short break (min)", &ui.pending_short_min, 1);
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Long break (min)",  &ui.pending_long_min,  1);
            ImGui::SetNextItemWidth(80); ImGui::InputInt("Long every N sessions", &ui.pending_cadence, 1);
            ui.pending_work_min  = std::clamp(ui.pending_work_min,  1, 120);
            ui.pending_short_min = std::clamp(ui.pending_short_min, 1, 60);
            ui.pending_long_min  = std::clamp(ui.pending_long_min,  1, 120);
            ui.pending_cadence   = std::clamp(ui.pending_cadence,
                                              POMODORO_MIN_CADENCE, POMODORO_MAX_CADENCE);
            ImGui::Checkbox("Auto-start next phase", &ui.pending_auto_start);
            ImGui::EndTabItem();
        }

        // ── Timers ──
        if (ImGui::BeginTabItem("Timers", nullptr, tab_flags(3))) {
            ImGui::Text("Custom presets");
            for (int i = 0; i < 5; ++i) {
                ImGui::SetNextItemWidth(80);
                ImGui::InputInt(std::format("Preset {} (min)", i + 1).c_str(),
                                &ui.pending_presets[i], 1);
                ui.pending_presets[i] = std::clamp(ui.pending_presets[i], 0, 1440);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    if (ImGui::Button("Apply")) {
        app.theme_mode        = ui.pending_theme;
        app.clock_view        = ui.pending_clock_view;
        app.analog_style      = ui.pending_analog;
        app.sound_on_expiry   = ui.pending_sound;
        app.pomodoro_work_secs  = ui.pending_work_min  * 60;
        app.pomodoro_short_secs = ui.pending_short_min * 60;
        app.pomodoro_long_secs  = ui.pending_long_min  * 60;
        app.pomodoro_cadence  = ui.pending_cadence;
        app.pomodoro_auto_start = ui.pending_auto_start;
        app.custom_preset_secs.clear();
        for (int i = 0; i < 5; ++i)
            if (ui.pending_presets[i] > 0)
                app.custom_preset_secs.push_back(ui.pending_presets[i] * 60);
        apply_imgui_theme(app.theme_mode, false);
        ui.dirty = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

// ─── Alarm firing (cross-platform) ───────────────────────────────────────────

static void check_alarms_cross_platform(App& app) {
    time_t t = std::time(nullptr);
    tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    int h = lt.tm_hour, m = lt.tm_min;
    int cur_min = h * 60 + m;

    if (cur_min != app.alarm_notified_minute) {
        app.alarm_notified_minute = cur_min;
        for (auto& a : app.alarms) a.notified = false;
    }

    // dow: POSIX tm_wday: 0=Sun, we want Mon=0..Sun=6
    int dow_bit = (lt.tm_wday == 0) ? 6 : (lt.tm_wday - 1);

    for (auto& a : app.alarms) {
        if (a.notified || !a.enabled) continue;
        if (a.hour != h || a.minute != m) continue;
        bool matches = false;
        if (a.schedule == AlarmSchedule::Days)
            matches = (a.days_mask & (1 << dow_bit)) != 0;
        else
            matches = a.date_year == lt.tm_year + 1900 &&
                      a.date_month == lt.tm_mon + 1 &&
                      a.date_day == lt.tm_mday;
        if (!matches) continue;
        a.notified = true;
        // Platform beep
#ifdef _WIN32
        MessageBeep(MB_ICONASTERISK);
#else
        // Emit bell character; terminal may beep
        fputs("\a", stderr);
#endif
    }
}

// ─── Main render entry point ──────────────────────────────────────────────────

void render_app(App& app, UiState& ui) {
    const ThemePalette& pal = palette_for(app.theme_mode, false);

    // Full-screen borderless window
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(1.f);
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    render_toolbar(app, ui, pal);
    render_clock(app, ui, pal);
    render_stopwatch(app, ui, pal);
    render_timers(app, ui, pal);
    render_alarms(app, ui);

    render_add_alarm_modal(app, ui);
    render_add_alarm_popup(app, ui);
    render_settings_modal(app, ui);
    render_settings_popup(app, ui);

    check_alarms_cross_platform(app);

    ImGui::End();
}
