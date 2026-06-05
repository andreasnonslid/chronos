#include "ui_render.hpp"
using namespace std::chrono;

// ─── Title bar ───────────────────────────────────────────────────────────────

#ifdef CHRONOS_DEBUG_UI_OVERLAY
static void debug_set_ui_scale(UiState& ui, float scale) {
    float old_scale = std::max(ui.debug_ui_scale, 0.1f);
    float new_scale = std::clamp(scale, 1.0f, 2.5f);
    if (new_scale == ui.debug_ui_scale) return;

    if (ui.sdl_window) {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(ui.sdl_window, &w, &h);

        float ratio = new_scale / old_scale;
        int new_w = std::clamp((int)(w * ratio + 0.5f), 240, 1600);
        int new_h = std::clamp((int)(h * ratio + 0.5f), 180, 1400);
        SDL_SetWindowMinimumSize(ui.sdl_window,
                                 std::clamp((int)(240 * new_scale + 0.5f), 240, 600),
                                 std::clamp((int)(180 * new_scale + 0.5f), 180, 450));
        SDL_SetWindowSize(ui.sdl_window, new_w, new_h);
    }

    ui.debug_ui_scale = new_scale;
}
#endif

void render_titlebar(App& app, UiState& ui, const ThemePalette& pal) {
    auto& s = ImGui::GetStyle();
    float bar_h = ImGui::GetFrameHeightWithSpacing() + s.ItemSpacing.y;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_v4(pal.bar));
    ImGui::BeginChild("##titlebar", {0, bar_h}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 title_btn_size = {
        std::max(ImGui::CalcTextSize("Alrm").x, ImGui::CalcTextSize("Dbg").x) + s.FramePadding.x * 2.f,
        ImGui::GetFrameHeight()
    };

    // Section toggle buttons
    auto toggle_btn = [&](const char* label, bool active, int action) {
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button,        to_v4(pal.active));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_v4(pal.active));
        }
        if (ImGui::Button(label, title_btn_size)) {
            auto r = dispatch_action(app, action, steady_clock::now(), {});
            if (r.save_config) ui.dirty = true;
            if (r.set_topmost) platform_set_always_on_top(ui.sdl_window, app.topmost);
            if (r.apply_theme) apply_imgui_theme(app.theme_mode, false);
        }
        CHRONOS_DEBUG_ITEM(label, IM_COL32(255, 255, 255, 240));
        if (active) ImGui::PopStyleColor(2);
    };

    if (ImGui::BeginTable("##titlebar_layout", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::BeginChild("##titlebar_left_clip", {0, bar_h}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    toggle_btn("Pin",  app.topmost,     A_TOPMOST); ImGui::SameLine();
    toggle_btn("Clk",  app.show_clk,    A_SHOW_CLK); ImGui::SameLine();
    toggle_btn("SW",   app.show_sw,     A_SHOW_SW); ImGui::SameLine();
    toggle_btn("Tmr",  app.show_tmr,    A_SHOW_TMR); ImGui::SameLine();
    toggle_btn("Alrm", app.show_alarms, A_SHOW_ALARMS);

#ifdef CHRONOS_DEBUG_UI_OVERLAY
    ImGui::SameLine();
    bool debug_was_visible = ui.debug_overlay_visible;
    if (debug_was_visible) {
        ImGui::PushStyleColor(ImGuiCol_Button,        to_v4(pal.active));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_v4(pal.active));
    }
    if (ImGui::Button("Dbg", title_btn_size)) {
        ui.debug_overlay_visible = !ui.debug_overlay_visible;
    }
    CHRONOS_DEBUG_ITEM("debug toggle", IM_COL32(255, 255, 255, 240));
    if (debug_was_visible) ImGui::PopStyleColor(2);
    ImGui::SameLine();
    if (ImGui::Button("-##dbg_scale", title_btn_size)) {
        debug_set_ui_scale(ui, ui.debug_ui_scale - 0.1f);
    }
    CHRONOS_DEBUG_ITEM("debug scale -", IM_COL32(255, 140, 0, 255));
    ImGui::SameLine();
    if (ImGui::Button("+##dbg_scale", title_btn_size)) {
        debug_set_ui_scale(ui, ui.debug_ui_scale + 0.1f);
    }
    CHRONOS_DEBUG_ITEM("debug scale +", IM_COL32(255, 140, 0, 255));
#endif

    // Right-align ⚙ and ×
    // UTF-8: ⚙ = \xe2\x9a\x99 (U+2699), × = \xc3\x97 (U+00D7)
    ImGui::EndChild();
    ImGui::TableSetColumnIndex(1);

    if (ImGui::Button("_##minimize_tray", title_btn_size)) {
        ui.minimize_to_tray_requested = true;
    }
    CHRONOS_DEBUG_ITEM("tray minimize", IM_COL32(80, 220, 255, 255));
    ImGui::SameLine();
    if (ImGui::Button("\xe2\x9a\x99", title_btn_size)) {
        if (!ui.show_settings) ui.settings_initialized = false;
        ui.show_settings = true;
        ui.settings_tab  = 0;
    }
    CHRONOS_DEBUG_ITEM("settings", IM_COL32(80, 220, 255, 255));
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.8f, 0.2f, 0.2f, 1.f});
    if (ImGui::Button("\xc3\x97", title_btn_size)) ui.close_requested = true;
    CHRONOS_DEBUG_ITEM("close", IM_COL32(255, 80, 80, 255));
    ImGui::PopStyleColor();
    ImGui::EndTable();
    }

    // Window drag: active when mouse is pressed in the bar but not over any item.
    ImVec2 bar_min = ImGui::GetWindowPos();
    ImVec2 bar_max = {bar_min.x + ImGui::GetWindowWidth(), bar_min.y + bar_h};
    bool in_bar = ImGui::IsMouseHoveringRect(bar_min, bar_max, false);
    if (in_bar && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        platform_begin_window_drag(ui.sdl_window);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
