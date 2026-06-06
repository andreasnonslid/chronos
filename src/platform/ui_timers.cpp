#include "ui_render.hpp"
using namespace std::chrono;

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

void render_timers(App& app, UiState& ui, const ThemePalette& pal) {
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
