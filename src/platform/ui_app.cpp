#include "ui_app.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cmath>
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
#include "platform_window.hpp"
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

#ifdef CHRONOS_DEBUG_UI_OVERLAY
static const bool* g_debug_flag = nullptr;

static void debug_last_item(const char* label, ImU32 color = IM_COL32(255, 255, 255, 230)) {
    (void)label;
    if (!g_debug_flag || !*g_debug_flag) return;
    if (!ImGui::IsItemVisible()) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    dl->AddRect(min, max, color, 0.f, 0, 2.f);
}

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
#define CHRONOS_DEBUG_ITEM(label, color) debug_last_item(label, color)
#else
#define CHRONOS_DEBUG_ITEM(label, color) ((void)0)
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

// ─── String helpers ───────────────────────────────────────────────────────────

static std::string ws(const std::wstring& w) { return wide_to_utf8(w); }

// ─── Title bar ───────────────────────────────────────────────────────────────
// Replaces the OS title bar (window is SDL_WINDOW_BORDERLESS).
// Contains: drag region, section toggles, settings ⚙, close ×.

static void render_titlebar(App& app, UiState& ui, const ThemePalette& pal) {
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

// ─── Clock ───────────────────────────────────────────────────────────────────

static void draw_centered_clock_text(const ImVec2& min, const ImVec2& max,
                                     const std::string& text, float max_scale) {
    ImVec2 rect_size = {max.x - min.x, max.y - min.y};
    float scale = max_scale;
    ImVec2 text_size{};
    for (; scale > 0.75f; scale -= 0.05f) {
        ImGui::SetWindowFontScale(scale);
        text_size = ImGui::CalcTextSize(text.c_str(), nullptr, false, rect_size.x - 12.f);
        if (text_size.x <= rect_size.x - 12.f && text_size.y <= rect_size.y - 8.f) break;
    }
    ImGui::SetCursorScreenPos({
        min.x + std::max(0.f, (rect_size.x - text_size.x) * 0.5f),
        min.y + std::max(0.f, (rect_size.y - text_size.y) * 0.5f)
    });
    ImGui::PushTextWrapPos(max.x - 6.f);
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopTextWrapPos();
    ImGui::SetWindowFontScale(1.f);
}

static void render_analog_clock_in_rect(const ImVec2& min, const ImVec2& max,
                                        const App& app, const ThemePalette& pal,
                                        int h, int m, int s) {
    ImVec2 size = {max.x - min.x, max.y - min.y};
    float radius = std::max(0.f, std::min(size.x, size.y) * 0.5f - 6.f);
    ImVec2 center = {min.x + size.x * 0.5f, min.y + size.y * 0.5f};
    draw_analog_clock_imgui(ImGui::GetWindowDrawList(), center.x, center.y, radius,
                            app.analog_style, pal, h, m, s);
}

static void render_clock(App& app, UiState& ui, const ThemePalette& pal, float preferred_height) {
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
    int h12 = h % 12; if (h12 == 0) h12 = 12;

    bool has_analog = clock_view_has_analog(app.clock_view);
    ImVec2 clock_min = ImGui::GetCursorScreenPos();
    float clock_width = ImGui::GetContentRegionAvail().x;
    float clock_height = std::max(80.f, preferred_height);
    ImVec2 clock_max = {clock_min.x + clock_width, clock_min.y + clock_height};

    ImGui::InvisibleButton("##clock_widget", {clock_width, clock_height});
    bool clock_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

    if (clock_view_is_mixed(app.clock_view)) {
        std::string txt24 = std::format("{:02}:{:02}:{:02}", h, m, s);
        std::string txt12 = std::format("{}:{:02}:{:02} {}", h12, m, s, h < 12 ? "AM" : "PM");
        bool split_horizontal = app.clock_split_mode == ClockSplitMode::Horizontal ||
            (app.clock_split_mode == ClockSplitMode::Auto && clock_width >= clock_height * 1.25f);
        float split_ratio = std::clamp(app.clock_split_pct, 20, 80) / 100.f;
        ImVec2 a_min = clock_min;
        ImVec2 a_max = split_horizontal
            ? ImVec2{clock_min.x + clock_width * split_ratio, clock_max.y}
            : ImVec2{clock_max.x, clock_min.y + clock_height * split_ratio};
        ImVec2 b_min = split_horizontal
            ? ImVec2{a_max.x, clock_min.y}
            : ImVec2{clock_min.x, a_max.y};
        ImVec2 b_max = clock_max;

        if (has_analog) {
            render_analog_clock_in_rect(a_min, a_max, app, pal, h, m, s);
            draw_centered_clock_text(b_min, b_max,
                app.clock_view == ClockView::Mixed_AnalogIntlLocal
                    ? std::format("{}\n{}", txt24, txt12)
                    : format_clock_text(app.clock_view, h, m, s),
                2.4f);
        } else {
            draw_centered_clock_text(a_min, a_max, txt24, 2.6f);
            draw_centered_clock_text(b_min, b_max, txt12, 2.6f);
        }

        float gutter = 8.f;
        ImVec2 split_min = split_horizontal
            ? ImVec2{a_max.x - gutter * 0.5f, clock_min.y}
            : ImVec2{clock_min.x, a_max.y - gutter * 0.5f};
        ImVec2 split_max = split_horizontal
            ? ImVec2{a_max.x + gutter * 0.5f, clock_max.y}
            : ImVec2{clock_max.x, a_max.y + gutter * 0.5f};
        ImGui::SetCursorScreenPos(split_min);
        ImGui::InvisibleButton("##clock_splitter", {split_max.x - split_min.x, split_max.y - split_min.y});
        bool splitter_used = ImGui::IsItemHovered() || ImGui::IsItemActive();
        if (ImGui::IsItemActive()) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            float raw_pct = split_horizontal
                ? (mouse.x - clock_min.x) / std::max(clock_width, 1.f) * 100.f
                : (mouse.y - clock_min.y) / std::max(clock_height, 1.f) * 100.f;
            int next_pct = std::clamp((int)std::lroundf(raw_pct), 20, 80);
            if (next_pct != app.clock_split_pct) {
                app.clock_split_pct = next_pct;
                ui.dirty = true;
            }
        }
#ifdef CHRONOS_DEBUG_UI_OVERLAY
        if (g_debug_overlay_visible) {
            ImGui::GetForegroundDrawList()->AddLine(
                split_horizontal ? ImVec2{a_max.x, clock_min.y} : ImVec2{clock_min.x, a_max.y},
                split_horizontal ? ImVec2{a_max.x, clock_max.y} : ImVec2{clock_max.x, a_max.y},
                IM_COL32(255, 210, 0, 140), 1.5f);
        }
#endif
        if (splitter_used) clock_clicked = false;
    } else if (has_analog) {
        render_analog_clock_in_rect(clock_min, clock_max, app, pal, h, m, s);
    } else {
        draw_centered_clock_text(clock_min, clock_max, format_clock_text(app.clock_view, h, m, s), 3.2f);
    }
#ifdef CHRONOS_DEBUG_UI_OVERLAY
    if (g_debug_overlay_visible) {
        ImGui::GetForegroundDrawList()->AddRect(clock_min, clock_max, IM_COL32(255, 210, 0, 255), 0.f, 0, 2.f);
    }
#endif
    if (clock_clicked) {
        auto r = dispatch_action(app, A_CLK_CYCLE, now, {});
        if (r.save_config) ui.dirty = true;
    }
    ImGui::SetCursorScreenPos({clock_min.x, clock_max.y});
}

// ─── Stopwatch ───────────────────────────────────────────────────────────────

static void render_stopwatch(App& app, [[maybe_unused]] UiState& ui, [[maybe_unused]] const ThemePalette& pal) {
    if (!app.show_sw) return;
    auto now = steady_clock::now();
    std::string elapsed = format_stopwatch_display(app.sw.elapsed(now));

    ImGui::SetWindowFontScale(1.5f);
    float tw = ImGui::CalcTextSize(elapsed.c_str()).x;
    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw) / 2.f);
    ImGui::TextUnformatted(elapsed.c_str());
    ImGui::SetWindowFontScale(1.f);

    const char* start_label = app.sw.is_running() ? "Stop" : "Start";
    float button_w = ImGui::CalcTextSize(start_label).x + ImGui::GetStyle().FramePadding.x * 2.f
        + ImGui::CalcTextSize("Lap").x + ImGui::GetStyle().FramePadding.x * 2.f
        + ImGui::CalcTextSize("Reset").x + ImGui::GetStyle().FramePadding.x * 2.f
        + ImGui::GetStyle().ItemSpacing.x * 2.f;
    float button_avail = ImGui::GetContentRegionAvail().x;
    if (button_w < button_avail) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (button_avail - button_w) * 0.5f);
    }

    auto btn = [&](const char* label, int action, bool same_line_after) {
        if (ImGui::Button(label)) {
            auto r = dispatch_action(app, action, now, {});
            if (r.save_config) ui.dirty = true;
        }
        CHRONOS_DEBUG_ITEM(label, IM_COL32(0, 255, 160, 255));
        if (same_line_after) ImGui::SameLine();
    };

    btn(start_label, A_SW_START, true);
    btn("Lap",   A_SW_LAP, true);
    btn("Reset", A_SW_RESET, false);
}

// ─── Timers ──────────────────────────────────────────────────────────────────

static void render_timer_scroll_value(App& app, UiState& ui, int timer_idx,
                                      const char* id, const char* text,
                                      int down_off, int up_off,
                                      steady_clock::time_point now) {
    const ImGuiStyle& s = ImGui::GetStyle();
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 size = {text_size.x + s.FramePadding.x * 2.f, ImGui::GetFrameHeight()};
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    if (hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.f) {
            int off = wheel > 0.f ? up_off : down_off;
            auto r = dispatch_action(app, A_TMR_BASE + timer_idx * TMR_STRIDE + off, now, {});
            if (r.save_config) ui.dirty = true;
        }
    }
    ImU32 color = ImGui::GetColorU32(hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    ImGui::GetWindowDrawList()->AddText({pos.x + s.FramePadding.x, pos.y + s.FramePadding.y},
                                        color, text);
    CHRONOS_DEBUG_ITEM(id, IM_COL32(255, 170, 80, 255));
}

static void render_timer_edit_scroll(App& app, UiState& ui, int timer_idx,
                                     std::chrono::seconds dur,
                                     steady_clock::time_point now) {
    auto total = dur.count();
    int h = (int)(total / 3600);
    int m = (int)((total / 60) % 60);
    int s = (int)(total % 60);
    std::string hh = std::format("{}", h);
    std::string mm = std::format("{:02}", m);
    std::string ss = std::format("{:02}", s);
    render_timer_scroll_value(app, ui, timer_idx, "##timer_h", hh.c_str(), A_TMR_HDN, A_TMR_HUP, now);
    ImGui::SameLine(0.f, 0.f);
    ImGui::TextUnformatted(":");
    ImGui::SameLine(0.f, 0.f);
    render_timer_scroll_value(app, ui, timer_idx, "##timer_m", mm.c_str(), A_TMR_MDN, A_TMR_MUP, now);
    ImGui::SameLine(0.f, 0.f);
    ImGui::TextUnformatted(":");
    ImGui::SameLine(0.f, 0.f);
    render_timer_scroll_value(app, ui, timer_idx, "##timer_s", ss.c_str(), A_TMR_SDN, A_TMR_SUP, now);
}

static ImVec2 timer_action_button_size() {
    const ImGuiStyle& s = ImGui::GetStyle();
    float text_w = std::max(ImGui::CalcTextSize("Start").x, ImGui::CalcTextSize("Reset").x);
    text_w = std::max(text_w, ImGui::CalcTextSize("Pomo").x);
    return {text_w + s.FramePadding.x * 2.f, ImGui::GetFrameHeight()};
}

static bool timer_action_slot(const char* label, const ImVec2& size, bool enabled,
                              [[maybe_unused]] const char* debug_label,
                              [[maybe_unused]] ImU32 debug_color) {
    if (enabled) {
        bool clicked = ImGui::Button(label, size);
        CHRONOS_DEBUG_ITEM(debug_label, debug_color);
        return clicked;
    }
    ImGui::InvisibleButton(label, size);
    return false;
}

static void render_timers(App& app, UiState& ui, const ThemePalette& pal) {
    if (!app.show_tmr) return;
    auto now = steady_clock::now();

    for (int i = 0; i < (int)app.timers.size(); ++i) {
        auto& ts = app.timers[i];
        bool running = ts.t.is_running();
        bool expired = ts.t.touched() && ts.t.expired(now);
        bool untouched = !ts.t.touched();

        ImGui::PushID(i);

        std::string lbl = ts.label.empty() ? std::format("Timer {}", i + 1) : ws(ts.label);
        std::string tstr = ws(format_timer_display(ts.t.remaining(now)));
        bool remove_timer = false;

        if (ImGui::BeginTable("##timer_row", 3,
                              ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("actions", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(lbl.c_str());

            ImGui::TableSetColumnIndex(1);
            if (running || ts.t.touched() || ts.pomodoro) {
                if (expired) ImGui::PushStyleColor(ImGuiCol_Text, to_v4(pal.expire));
                ImGui::TextUnformatted(tstr.c_str());
                if (expired) ImGui::PopStyleColor();
            } else {
                render_timer_edit_scroll(app, ui, i, ts.dur, now);
            }

            ImGui::TableSetColumnIndex(2);
            ImVec2 action_size = timer_action_button_size();
            auto tmr_act_do = [&](int off) {
                auto r = dispatch_action(app, A_TMR_BASE + i * TMR_STRIDE + off, now, {});
                if (r.save_config) ui.dirty = true;
            };
            if (timer_action_slot(running ? "Stop" : "Start", action_size, true,
                                  running ? "timer stop" : "timer start", IM_COL32(255, 100, 200, 255)))
                tmr_act_do(A_TMR_START);
            ImGui::SameLine();

            if (timer_action_slot("Reset", action_size, true, "timer reset", IM_COL32(255, 100, 200, 255)))
                tmr_act_do(A_TMR_RST);
            ImGui::SameLine();

            if (untouched) {
                if (ts.pomodoro) ImGui::PushStyleColor(ImGuiCol_Button, to_v4(pal.active));
                if (timer_action_slot("Pomo", action_size, true, "timer pomo", IM_COL32(255, 100, 200, 255)))
                    tmr_act_do(A_TMR_POMO);
                if (ts.pomodoro) ImGui::PopStyleColor();
            } else if (ts.pomodoro && ts.t.touched()) {
                if (timer_action_slot("Skip", action_size, true, "timer skip", IM_COL32(255, 100, 200, 255)))
                    tmr_act_do(A_TMR_SKIP);
            } else {
                timer_action_slot("##timer_action_spacer", action_size, false,
                                  "timer spacer", IM_COL32(255, 100, 200, 255));
            }

            if ((int)app.timers.size() > 1) {
                ImGui::SameLine();
                if (ImGui::SmallButton("-")) {
                    tmr_act_do(A_TMR_DEL);
                    remove_timer = true;
                }
                CHRONOS_DEBUG_ITEM("timer remove", IM_COL32(255, 100, 200, 255));
            }

            ImGui::EndTable();
        }

        if (remove_timer) {
            ImGui::PopID();
            break;
        }

        // Reserve a stable progress band so starting a timer does not resize the row.
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        float h = 4.f;
        if (ts.t.touched()) {
            float dur_ms = (float)duration_cast<milliseconds>(ts.dur).count();
            float rem_ms = (float)duration_cast<milliseconds>(ts.t.remaining(now)).count();
            float frac   = dur_ms > 0 ? std::clamp(rem_ms / dur_ms, 0.f, 1.f) : 0.f;
            ImU32 fill_col = expired ? to_u32(pal.expire, 0.4f) : to_u32(pal.fill, 0.6f);
            ImGui::GetWindowDrawList()->AddRectFilled(p, {p.x + w * frac, p.y + h}, fill_col);
        }
        ImGui::Dummy({0, h});

        ImGui::PopID();
    }

    // Add timer button
    if ((int)app.timers.size() < Config::MAX_TIMERS) {
        if (ImGui::SmallButton("+ Timer")) {
            int last = (int)app.timers.size() - 1;
            auto r = dispatch_action(app, A_TMR_BASE + last * TMR_STRIDE + A_TMR_ADD, steady_clock::now(), {});
            if (r.save_config) ui.dirty = true;
        }
        CHRONOS_DEBUG_ITEM("add timer", IM_COL32(255, 100, 200, 255));
    }
}

// ─── Alarms ──────────────────────────────────────────────────────────────────

static void render_alarms(App& app, UiState& ui) {
    if (!app.show_alarms) return;
    if (ImGui::BeginTable("##alarm_header", 2,
                          ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableSetupColumn("title", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Alarms");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::SmallButton("+ Add") && (int)app.alarms.size() < ALARM_MAX_COUNT) {
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
        CHRONOS_DEBUG_ITEM("add alarm", IM_COL32(180, 120, 255, 255));
        ImGui::EndTable();
    }

    if (app.alarms.empty()) {
        ImGui::TextDisabled("No alarms set");
    } else if (ImGui::BeginTable("##alarm_rows", 4,
                                 ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableSetupColumn("enabled", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed);
        for (int i = 0; i < (int)app.alarms.size(); ++i) {
            auto& a = app.alarms[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            bool en = a.enabled;
            if (ImGui::Checkbox("##en", &en)) {
                auto r = dispatch_action(app, A_ALARM_TOGGLE + i, steady_clock::now(), {});
                if (r.save_config) ui.dirty = true;
            }
            CHRONOS_DEBUG_ITEM("alarm enabled", IM_COL32(180, 120, 255, 255));
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(a.name.empty() ? "(unnamed)" : a.name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%02d:%02d", a.hour, a.minute);
            ImGui::TableSetColumnIndex(3);
            if (ImGui::SmallButton("Del")) {
                auto r = dispatch_action(app, A_ALARM_DEL + i, steady_clock::now(), {});
                if (r.save_config) ui.dirty = true;
                ImGui::PopID();
                break;
            }
            CHRONOS_DEBUG_ITEM("alarm delete", IM_COL32(180, 120, 255, 255));
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// ─── Add-alarm window ────────────────────────────────────────────────────────

static void render_add_alarm_window(App& app, UiState& ui) {
    if (!ui.show_add_alarm) return;

    ImGui::TextUnformatted("Add Alarm");
    ImGui::Separator();

    ImGui::InputText("Name", ui.alarm_name, sizeof(ui.alarm_name));
    CHRONOS_DEBUG_ITEM("alarm name", IM_COL32(180, 120, 255, 255));

    {
        const float btn  = ImGui::GetFrameHeight();
        const float sp   = ImGui::GetStyle().ItemSpacing.x;
        const float numw = ImGui::CalcTextSize("00").x + sp * 2.f;
        const float colw = ImGui::CalcTextSize(":").x;
        const float total = btn * 4.f + numw * 2.f + colw + sp * 6.f;
        const float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - total) * 0.5f);

        if (ImGui::Button("-##h", {btn, btn})) ui.alarm_hour = (ui.alarm_hour + 23) % 24;
        ImGui::SameLine();
        ImGui::Text("%02d", ui.alarm_hour);
        ImGui::SameLine();
        if (ImGui::Button("+##h", {btn, btn})) ui.alarm_hour = (ui.alarm_hour + 1) % 24;
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        if (ImGui::Button("-##m", {btn, btn})) ui.alarm_minute = (ui.alarm_minute + 59) % 60;
        ImGui::SameLine();
        ImGui::Text("%02d", ui.alarm_minute);
        ImGui::SameLine();
        if (ImGui::Button("+##m", {btn, btn})) ui.alarm_minute = (ui.alarm_minute + 1) % 60;
    }
    CHRONOS_DEBUG_ITEM("alarm time", IM_COL32(180, 120, 255, 255));

    {
        int mode = ui.alarm_days_mode ? 1 : 0;
        ImGui::RadioButton("Days of week", &mode, 1);
        CHRONOS_DEBUG_ITEM("days mode", IM_COL32(180, 120, 255, 255));
        ImGui::SameLine();
        ImGui::RadioButton("Specific date", &mode, 0);
        CHRONOS_DEBUG_ITEM("date mode", IM_COL32(180, 120, 255, 255));
        ui.alarm_days_mode = (mode != 0);
    }

    if (ui.alarm_days_mode) {
        const char* day_names[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
        for (int d = 0; d < 7; ++d) {
            ImGui::Checkbox(day_names[d], &ui.alarm_days[d]);
            CHRONOS_DEBUG_ITEM(day_names[d], IM_COL32(180, 120, 255, 255));
            if (d < 6) ImGui::SameLine();
        }
    } else {
        ImGui::SetNextItemWidth(70); ImGui::InputInt("Year",  &ui.alarm_year,  1);
        CHRONOS_DEBUG_ITEM("alarm year", IM_COL32(180, 120, 255, 255));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50); ImGui::InputInt("Month", &ui.alarm_month, 1);
        CHRONOS_DEBUG_ITEM("alarm month", IM_COL32(180, 120, 255, 255));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50); ImGui::InputInt("Day",   &ui.alarm_day,   1);
        CHRONOS_DEBUG_ITEM("alarm day", IM_COL32(180, 120, 255, 255));
        ui.alarm_month = std::clamp(ui.alarm_month, 1, 12);
        ui.alarm_day   = std::clamp(ui.alarm_day,   1, 31);
    }

    ImGui::Separator();
    if (ImGui::Button("OK")) {
        Alarm a;
        a.name    = ui.alarm_name;
        a.hour    = ui.alarm_hour;
        a.minute  = ui.alarm_minute;
        a.enabled = true;
        if (ui.alarm_days_mode) {
            a.schedule  = AlarmSchedule::Days;
            a.days_mask = 0;
            for (int d = 0; d < 7; ++d)
                if (ui.alarm_days[d]) a.days_mask |= (1 << d);
        } else {
            a.schedule    = AlarmSchedule::Date;
            a.date_year   = ui.alarm_year;
            a.date_month  = ui.alarm_month;
            a.date_day    = ui.alarm_day;
        }
        app.alarms.push_back(a);
        ui.dirty = true;
        ui.show_add_alarm = false;
    }
    CHRONOS_DEBUG_ITEM("alarm ok", IM_COL32(180, 120, 255, 255));
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) ui.show_add_alarm = false;
    CHRONOS_DEBUG_ITEM("alarm cancel", IM_COL32(180, 120, 255, 255));
}

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

static void render_settings_window(App& app, UiState& ui) {
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

// ─── Main render entry point ──────────────────────────────────────────────────

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
