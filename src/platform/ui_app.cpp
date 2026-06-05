#include "ui_render.hpp"
using namespace std::chrono;

// ─── Theme ───────────────────────────────────────────────────────────────────

#ifdef CHRONOS_DEBUG_UI_OVERLAY
const bool* g_debug_flag = nullptr;

void debug_last_item(const char* label, ImU32 color) {
    (void)label;
    if (!g_debug_flag || !*g_debug_flag) return;
    if (!ImGui::IsItemVisible()) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    dl->AddRect(min, max, color, 0.f, 0, 2.f);
}
#endif

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

// ─── Debug overlay helpers ────────────────────────────────────────────────────

#ifdef CHRONOS_DEBUG_UI_OVERLAY
static void debug_rect(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 color, const char* label) {
    (void)label;
    if (b.x <= a.x || b.y <= a.y) return;
    dl->AddRect(a, b, color, 0.f, 0, 2.f);
}

static void debug_edge_overlay(ImDrawList* dl, const ImVec2& size) {
    constexpr float edge = 10.f;
    dl->AddRectFilled({0, 0}, {size.x, edge}, IM_COL32(255, 210, 0, 50));
    dl->AddRectFilled({0, size.y - edge}, {size.x, size.y}, IM_COL32(0, 255, 80, 50));
    dl->AddRectFilled({0, 0}, {edge, size.y}, IM_COL32(0, 200, 255, 50));
    dl->AddRectFilled({size.x - edge, 0}, {size.x, size.y}, IM_COL32(255, 0, 200, 50));
    dl->AddRect({0, 0}, {size.x, size.y}, IM_COL32(255, 255, 255, 220), 0.f, 0, 2.f);
}

static void debug_section(ImDrawList* dl, const ImVec2& before, const char* label, ImU32 color) {
    ImVec2 after = ImGui::GetCursorScreenPos();
    debug_rect(dl, before, {before.x + ImGui::GetWindowWidth(), after.y}, color, label);
}
#endif

// ─── Layout estimation ────────────────────────────────────────────────────────

static float estimate_post_clock_height(const App& app) {
    const ImGuiStyle& s = ImGui::GetStyle();
    float h = 0.f;
    float line = ImGui::GetTextLineHeight();
    float frame = ImGui::GetFrameHeight();

    if (app.show_sw) {
        h += line * 1.5f + s.ItemSpacing.y + frame + s.ItemSpacing.y;
    }

    if (app.show_tmr) {
        h += line + s.ItemSpacing.y;
        for (const auto& ts : app.timers) {
            (void)ts;
            h += frame;
            h += 4.f;
            h += s.ItemSpacing.y;
        }
    }

    if (app.show_alarms) {
        h += frame + s.ItemSpacing.y;
        h += app.alarms.empty()
            ? line + s.ItemSpacing.y
            : (frame + s.ItemSpacing.y) * (float)app.alarms.size();
    }

    h += s.ItemSpacing.y;
    return h;
}

// ─── Main render entry point ──────────────────────────────────────────────────

void render_app(App& app, UiState& ui) {
    const ThemePalette& pal = palette_for(app.theme_mode, false);

    // Full-screen borderless window
    ImGuiIO& io = ImGui::GetIO();
#ifdef CHRONOS_DEBUG_UI_OVERLAY
    io.FontGlobalScale = ui.debug_ui_scale;
#else
    io.FontGlobalScale = 1.f;
#endif
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(1.f);
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

#ifdef CHRONOS_DEBUG_UI_OVERLAY
    g_debug_flag = &ui.debug_overlay_visible;
    ImDrawList* debug_dl = ImGui::GetForegroundDrawList();
    if (ui.debug_overlay_visible) debug_edge_overlay(debug_dl, io.DisplaySize);
    ImVec2 dbg_before = ImGui::GetCursorScreenPos();
    auto dbg_section = [&](const char* name, ImU32 color) {
        if (ui.debug_overlay_visible) debug_section(debug_dl, dbg_before, name, color);
        dbg_before = ImGui::GetCursorScreenPos();
    };
#else
    auto dbg_section = [](const char*, ImU32) {};
#endif
    render_titlebar(app, ui, pal);
    dbg_section("titlebar", IM_COL32(80, 160, 255, 255));
    if (ui.show_add_alarm) {
        render_add_alarm_window(app, ui);
        dbg_section("add alarm", IM_COL32(180, 120, 255, 255));
    } else if (ui.show_settings) {
        render_settings_window(app, ui);
        dbg_section("settings", IM_COL32(80, 220, 255, 255));
    } else {
        float clock_height = app.show_clk
            ? std::max(80.f, ImGui::GetContentRegionAvail().y - estimate_post_clock_height(app))
            : 0.f;
        render_clock(app, ui, pal, clock_height);
        dbg_section("clock", IM_COL32(255, 200, 50, 255));
        render_stopwatch(app, ui, pal);
        dbg_section("stopwatch", IM_COL32(0, 255, 160, 255));
        render_timers(app, ui, pal);
        dbg_section("timers", IM_COL32(255, 100, 200, 255));
        render_alarms(app, ui);
        dbg_section("alarms", IM_COL32(180, 120, 255, 255));
    }

    ImGui::End();
}
