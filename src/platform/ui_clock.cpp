#include "ui_render.hpp"
using namespace std::chrono;

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

void render_clock(App& app, UiState& ui, const ThemePalette& pal, float preferred_height) {
    if (!app.show_clk) return;

    auto now = steady_clock::now();
    tm lt = local_tm(std::time(nullptr));
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
        if (debug_flag && *debug_flag) {
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
    if (debug_flag && *debug_flag) {
        ImGui::GetForegroundDrawList()->AddRect(clock_min, clock_max, IM_COL32(255, 210, 0, 255), 0.f, 0, 2.f);
    }
#endif
    if (clock_clicked) {
        auto r = dispatch_action(app, A_CLK_CYCLE, now, {});
        if (r.save_config) ui.dirty = true;
    }
    ImGui::SetCursorScreenPos({clock_min.x, clock_max.y});
}
