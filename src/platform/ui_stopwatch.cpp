#include "ui_render.hpp"
using namespace std::chrono;

// ─── Stopwatch ───────────────────────────────────────────────────────────────

void render_stopwatch(App& app, [[maybe_unused]] UiState& ui, [[maybe_unused]] const ThemePalette& pal) {
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
