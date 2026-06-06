#include "ui_render.hpp"
using namespace std::chrono;

// ─── Shared helpers ───────────────────────────────────────────────────────────

static ImVec2 titlebar_btn_size() {
    const ImGuiStyle& s = ImGui::GetStyle();
    float w = std::max(ImGui::CalcTextSize("Alrm").x, ImGui::CalcTextSize("Dbg").x)
              + s.FramePadding.x * 2.f;
    return {w, ImGui::GetFrameHeight()};
}

static void render_toggle_btn(const char* label, bool active, int action,
                               const ImVec2& size, const ThemePalette& pal,
                               App& app, UiState& ui) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button,        to_v4(pal.active));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_v4(pal.active));
    }
    if (ImGui::Button(label, size)) {
        auto r = dispatch_action(app, action, steady_clock::now(), {});
        if (r.save_config) ui.dirty = true;
        if (r.set_topmost) platform_set_always_on_top(ui.sdl_window, app.topmost);
        if (r.apply_theme) apply_imgui_theme(app.theme_mode, false);
    }
    CHRONOS_DEBUG_ITEM(label, IM_COL32(255, 255, 255, 240));
    if (active) ImGui::PopStyleColor(2);
}

// ─── UI scale (Ctrl+/-) ───────────────────────────────────────────────────────

void adjust_ui_scale(UiState& ui, float delta) {
    float old_scale = std::max(ui.ui_scale, 0.1f);
    float new_scale = std::clamp(old_scale + delta, 0.5f, 3.0f);
    if (new_scale == old_scale) return;
#ifdef CHRONOS_DEBUG_UI_OVERLAY
    if (ui.sdl_window) {
        int w = 0, h = 0;
        SDL_GetWindowSize(ui.sdl_window, &w, &h);
        float ratio = new_scale / old_scale;
        int new_w = std::clamp((int)(w * ratio + 0.5f), 240, 1600);
        int new_h = std::clamp((int)(h * ratio + 0.5f), 180, 1400);
        SDL_SetWindowMinimumSize(ui.sdl_window,
                                 std::clamp((int)(240 * new_scale + 0.5f), 240, 600),
                                 std::clamp((int)(180 * new_scale + 0.5f), 180, 450));
        SDL_SetWindowSize(ui.sdl_window, new_w, new_h);
    }
#endif
    ui.ui_scale = new_scale;
}

// ─── Title bar ───────────────────────────────────────────────────────────────

void render_titlebar(UiState& ui, const ThemePalette& pal) {
    auto& s = ImGui::GetStyle();
    float bar_h = ImGui::GetFrameHeightWithSpacing() + s.ItemSpacing.y;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_v4(pal.bar));
    ImGui::BeginChild("##titlebar", {0, bar_h}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 btn_size = titlebar_btn_size();

    if (ImGui::BeginTable("##titlebar_layout", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("left",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();

        // Left: hamburger button
        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("##titlebar_left_clip", {0, bar_h}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (ui.toolbar_strip_open) {
            ImGui::PushStyleColor(ImGuiCol_Button,        to_v4(pal.active));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_v4(pal.active));
        }
        ImGui::Button("=", btn_size);  // hamburger placeholder
        CHRONOS_DEBUG_ITEM("hamburger", IM_COL32(255, 255, 255, 240));
        if (ui.toolbar_strip_open)
            ImGui::PopStyleColor(2);
        ui.toolbar_hamburger_hovered = ImGui::IsItemHovered();
        ImGui::EndChild();

        // Right: minimize / settings / close
        ImGui::TableSetColumnIndex(1);
        if (ImGui::Button("_##min", btn_size))
            ui.minimize_to_tray_requested = true;
        CHRONOS_DEBUG_ITEM("tray minimize", IM_COL32(80, 220, 255, 255));
        ImGui::SameLine();
        if (ImGui::Button("Cfg", btn_size)) {
            if (!ui.show_settings) ui.settings_initialized = false;
            ui.show_settings = true;
            ui.settings_tab  = 0;
        }
        CHRONOS_DEBUG_ITEM("settings", IM_COL32(80, 220, 255, 255));
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.8f, 0.2f, 0.2f, 1.f});
        if (ImGui::Button("X", btn_size)) ui.close_requested = true;
        CHRONOS_DEBUG_ITEM("close", IM_COL32(255, 80, 80, 255));
        ImGui::PopStyleColor();

        ImGui::EndTable();
    }

    // Store strip anchor (screen-space pos just below this bar)
    ImVec2 bar_min = ImGui::GetWindowPos();
    ui.toolbar_strip_screen_x = bar_min.x;
    ui.toolbar_strip_screen_y = bar_min.y + bar_h;

    // Window drag: active when mouse is in the bar but not over a button
    ImVec2 bar_max = {bar_min.x + ImGui::GetWindowWidth(), bar_min.y + bar_h};
    if (ImGui::IsMouseHoveringRect(bar_min, bar_max, false) &&
        !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        platform_begin_window_drag(ui.sdl_window);

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Toolbar strip (hover overflow) ──────────────────────────────────────────

void render_toolbar_strip(App& app, UiState& ui, const ThemePalette& pal) {
    float dt = ImGui::GetIO().DeltaTime;

    if (ui.toolbar_hamburger_hovered) {
        ui.toolbar_strip_open = true;
        ui.toolbar_strip_close_timer = 0.25f;
    }

    if (!ui.toolbar_strip_open) return;

    ImGui::SetNextWindowPos({ui.toolbar_strip_screen_x, ui.toolbar_strip_screen_y});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, to_v4(pal.bar));
    ImGui::PushStyleColor(ImGuiCol_Border,   to_v4(pal.divider));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);

    ImGui::Begin("##toolbar_strip", nullptr,
        ImGuiWindowFlags_NoTitleBar        | ImGuiWindowFlags_NoResize         |
        ImGuiWindowFlags_NoMove            | ImGuiWindowFlags_NoScrollbar      |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings  |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav             | ImGuiWindowFlags_AlwaysAutoResize);

    ImVec2 btn_size = titlebar_btn_size();

    render_toggle_btn("Pin",  app.topmost,     A_TOPMOST,     btn_size, pal, app, ui); ImGui::SameLine();
    render_toggle_btn("Clk",  app.show_clk,    A_SHOW_CLK,    btn_size, pal, app, ui); ImGui::SameLine();
    render_toggle_btn("SW",   app.show_sw,     A_SHOW_SW,     btn_size, pal, app, ui); ImGui::SameLine();
    render_toggle_btn("Tmr",  app.show_tmr,    A_SHOW_TMR,    btn_size, pal, app, ui); ImGui::SameLine();
    render_toggle_btn("Alrm", app.show_alarms, A_SHOW_ALARMS, btn_size, pal, app, ui);

#ifdef CHRONOS_DEBUG_UI_OVERLAY
    ImGui::SameLine();
    bool dbg = ui.debug_overlay_visible;
    if (dbg) {
        ImGui::PushStyleColor(ImGuiCol_Button,        to_v4(pal.active));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_v4(pal.active));
    }
    if (ImGui::Button("Dbg", btn_size)) ui.debug_overlay_visible = !ui.debug_overlay_visible;
    CHRONOS_DEBUG_ITEM("debug toggle", IM_COL32(255, 255, 255, 240));
    if (dbg) ImGui::PopStyleColor(2);
#endif

    // Keep strip open while mouse is over it
    ImVec2 smin = ImGui::GetWindowPos();
    ImVec2 smax = {smin.x + ImGui::GetWindowWidth(), smin.y + ImGui::GetWindowHeight()};
    bool strip_hovered = ImGui::IsMouseHoveringRect(smin, smax, false);

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    if (strip_hovered) {
        ui.toolbar_strip_close_timer = 0.25f;
    } else {
        ui.toolbar_strip_close_timer -= dt;
        if (ui.toolbar_strip_close_timer <= 0.f)
            ui.toolbar_strip_open = false;
    }
}
