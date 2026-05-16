#pragma once
#include <algorithm>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "actions.hpp"
#include "app.hpp"
#include "encoding.hpp"
#include "formatting.hpp"
#include "layout.hpp"
#include "ui_style.hpp"

namespace ui_scene {

struct RectI {
    int left;
    int top;
    int right;
    int bottom;
};

enum class Align { Left, Center, Right };

enum class OpKind { FillRect, Divider, Text, Button, Progress };

struct Op {
    OpKind kind = OpKind::FillRect;
    RectI rect{};
    std::string text;
    UiColor fill{};
    UiColor stroke{};
    UiColor text_color{};
    int radius_px = 0;
    Align align = Align::Left;
    int id = 0;
};

struct Scene {
    std::vector<Op> ops;
};

struct TimerSceneState {
    bool running = false;
    bool expired = false;
    double progress = 0.0;
    std::string text = "01:00";
    std::string label = "Timer 1";
};

struct AlarmSceneState {
    std::string label;
    std::string time_text;
    bool enabled = true;
};

struct MainSceneState {
    bool topmost = false;
    bool show_clock = true;
    bool show_stopwatch = true;
    bool show_timers = true;
    bool show_alarms = false;
    bool stopwatch_running = false;
    ClockView clock_view = ClockView::H24_HMS;
    std::string clock_text = "00:00:00";
    std::string stopwatch_text = "00:00.000";
    std::vector<TimerSceneState> timers{{}};
    std::vector<AlarmSceneState> alarms{};
};

inline int width(const RectI& r) { return r.right - r.left; }
inline int height(const RectI& r) { return r.bottom - r.top; }

inline void add_fill(Scene& scene, RectI rect, SurfacePaint paint) {
    Op op{};
    op.kind = OpKind::FillRect;
    op.rect = rect;
    op.fill = paint.background;
    op.stroke = paint.border;
    scene.ops.push_back(std::move(op));
}

inline void add_text(Scene& scene, RectI rect, std::string text, TextPaint paint, Align align = Align::Left,
                     int id = 0) {
    Op op{};
    op.kind = OpKind::Text;
    op.rect = rect;
    op.text = std::move(text);
    op.text_color = paint.color;
    op.align = align;
    op.id = id;
    scene.ops.push_back(std::move(op));
}

inline void add_divider(Scene& scene, int x0, int x1, int y, DividerPaint paint) {
    Op op{};
    op.kind = OpKind::Divider;
    op.rect = {x0, y, x1, y};
    op.stroke = paint.color;
    scene.ops.push_back(std::move(op));
}

inline void add_button(Scene& scene, RectI rect, std::string label, WidgetPaint paint, int id = 0) {
    Op op{};
    op.kind = OpKind::Button;
    op.rect = rect;
    op.text = std::move(label);
    op.fill = paint.fill;
    op.stroke = paint.border;
    op.text_color = paint.text;
    op.radius_px = paint.radius_px;
    op.align = Align::Center;
    op.id = id;
    scene.ops.push_back(std::move(op));
}

inline void add_progress(Scene& scene, RectI rect, ProgressPaint paint) {
    Op op{};
    op.kind = OpKind::Progress;
    op.rect = rect;
    op.fill = paint.fill;
    scene.ops.push_back(std::move(op));
}

inline void add_toolbar(Scene& scene, const Layout& layout, int client_w, const MainSceneState& state,
                        const UiMakers& ui) {
    add_fill(scene, {0, 0, client_w, layout.bar_h}, ui.surface(SurfaceConfig{.elevated = true}));

    struct ToolbarButton {
        int width;
        const char* label;
        bool active;
        int action;
    };
    ToolbarButton buttons[] = {
        {layout.w_pin, "Pin", state.topmost, A_TOPMOST},
        {layout.w_clk, "Clock", state.show_clock, A_SHOW_CLK},
        {layout.w_sw, "Stopwatch", state.show_stopwatch, A_SHOW_SW},
        {layout.w_tmr, "Timers", state.show_timers, A_SHOW_TMR},
        {layout.w_alm, "Alarms", state.show_alarms, A_SHOW_ALARMS},
        {layout.w_set, "Settings", false, A_SETTINGS},
    };

    int total_w = (static_cast<int>(std::size(buttons)) - 1) * layout.bar_gap;
    for (const auto& b : buttons) total_w += b.width;
    int x = (client_w - total_w) / 2;
    int y = (layout.bar_h - layout.btn_h) / 2;
    for (const auto& b : buttons) {
        add_button(scene, {x, y, x + b.width, y + layout.btn_h}, b.label,
                   ui.button(ButtonConfig{.active = b.active, .radius_px = layout.dpi_scale(6)}), b.action);
        x += b.width + layout.bar_gap;
    }
}

inline void add_clock(Scene& scene, const Layout& layout, int client_w, int& y, const MainSceneState& state,
                      const UiMakers& ui) {
    if (!state.show_clock) return;
    int h = effective_clk_h(layout, state.clock_view);
    add_divider(scene, 0, client_w, y, ui.divider());
    add_text(scene, {0, y, client_w, y + h}, state.clock_text, ui.text(), Align::Center, A_CLK_CYCLE);
    y += h;
}

inline void add_stopwatch(Scene& scene, const Layout& layout, int client_w, int& y, const MainSceneState& state,
                          const UiMakers& ui) {
    if (!state.show_stopwatch) return;
    add_divider(scene, 0, client_w, y, ui.divider());
    add_text(scene, {0, y + layout.dpi_scale(4), client_w, y + layout.dpi_scale(44)}, state.stopwatch_text, ui.text(),
             Align::Center);

    int bw = layout.dpi_scale(76);
    int gap = layout.dpi_scale(6);
    int bh = layout.dpi_scale(28);
    int x0 = (client_w - 3 * bw - 2 * gap) / 2;
    int by = y + layout.dpi_scale(50);
    add_button(scene, {x0, by, x0 + bw, by + bh}, state.stopwatch_running ? "Stop" : "Start",
               ui.button(ButtonConfig{.active = state.stopwatch_running, .radius_px = layout.dpi_scale(6)}),
               A_SW_START);
    add_button(scene, {x0 + bw + gap, by, x0 + 2 * bw + gap, by + bh}, "Lap",
               ui.button(ButtonConfig{.radius_px = layout.dpi_scale(6)}), A_SW_LAP);
    add_button(scene, {x0 + 2 * (bw + gap), by, x0 + 3 * bw + 2 * gap, by + bh}, "Reset",
               ui.button(ButtonConfig{.radius_px = layout.dpi_scale(6)}), A_SW_RESET);
    y += layout.sw_h;
}

inline void add_timer(Scene& scene, const Layout& layout, int client_w, int& y, int index, const TimerSceneState& timer,
                      const UiMakers& ui) {
    add_divider(scene, 0, client_w, y, ui.divider());

    int progress_w = std::clamp((int)(client_w * timer.progress), 0, client_w);
    if (progress_w > 0 || timer.expired) {
        add_progress(scene, {0, y, timer.expired ? client_w : progress_w, y + layout.tmr_h},
                     ui.progress(ProgressConfig{.expired = timer.expired}));
    }

    add_text(scene, {0, y + layout.dpi_scale(12), client_w, y + layout.dpi_scale(34)}, timer.label,
             ui.text(TextConfig{.tone = TextTone::Dim}), Align::Center);
    add_text(scene, {0, y + layout.dpi_scale(36), client_w, y + layout.dpi_scale(78)}, timer.text,
             ui.text(timer.expired ? TextConfig{.tone = TextTone::Danger} : TextConfig{}), Align::Center);

    int bw = layout.dpi_scale(86);
    int gap = layout.dpi_scale(6);
    int bh = layout.dpi_scale(28);
    int x0 = (client_w - 2 * bw - gap) / 2;
    int by = y + layout.dpi_scale(80);
    add_button(scene, {x0, by, x0 + bw, by + bh}, timer.running ? "Pause" : "Start",
               ui.button(ButtonConfig{.active = timer.running, .radius_px = layout.dpi_scale(6)}),
               tmr_act(index, A_TMR_START));
    add_button(scene, {x0 + bw + gap, by, x0 + 2 * bw + gap, by + bh}, "Reset",
               ui.button(ButtonConfig{.radius_px = layout.dpi_scale(6)}), tmr_act(index, A_TMR_RST));
    y += layout.tmr_h;
}

inline int main_scene_height(const Layout& layout, const MainSceneState& state) {
    int h = layout.bar_h;
    if (state.show_clock) h += effective_clk_h(layout, state.clock_view);
    if (state.show_stopwatch) h += layout.sw_h;
    if (state.show_timers) h += (int)state.timers.size() * layout.tmr_h;
    if (state.show_alarms)
        h += layout.alarm_header_h + std::max(1, (int)state.alarms.size()) * layout.alarm_row_h;
    return h;
}

inline void add_alarms(Scene& scene, const Layout& layout, int client_w, int& y, const MainSceneState& state,
                       const UiMakers& ui) {
    if (!state.show_alarms) return;
    add_divider(scene, 0, client_w, y, ui.divider());
    int header_bottom = y + layout.alarm_header_h;
    add_text(scene, {0, y, client_w / 2, header_bottom}, "Alarms", ui.text(), Align::Left);
    int bw = layout.dpi_scale(40);
    add_button(scene, {client_w - bw - layout.dpi_scale(4), y + layout.dpi_scale(7), client_w - layout.dpi_scale(4),
                       header_bottom - layout.dpi_scale(7)},
               "Add", ui.button(ButtonConfig{.radius_px = layout.dpi_scale(4)}), A_ALARM_ADD);
    y = header_bottom;
    if (state.alarms.empty()) {
        add_text(scene, {0, y, client_w, y + layout.alarm_row_h}, "No alarms", ui.text(TextConfig{.tone = TextTone::Dim}),
                 Align::Center);
        y += layout.alarm_row_h;
    } else {
        for (int i = 0; i < (int)state.alarms.size(); ++i) {
            const auto& alarm = state.alarms[i];
            int row_bottom = y + layout.alarm_row_h;
            add_text(scene, {layout.dpi_scale(4), y, client_w / 2, row_bottom}, alarm.time_text, ui.text(), Align::Left);
            add_text(scene, {client_w / 2, y, client_w - bw - layout.dpi_scale(8), row_bottom},
                     alarm.label.empty() ? "Alarm" : alarm.label,
                     ui.text(alarm.enabled ? TextConfig{} : TextConfig{.tone = TextTone::Dim}), Align::Left);
            add_button(scene,
                       {client_w - bw - layout.dpi_scale(4), y + layout.dpi_scale(4), client_w - layout.dpi_scale(4),
                        row_bottom - layout.dpi_scale(4)},
                       alarm.enabled ? "On" : "Off",
                       ui.button(ButtonConfig{.active = alarm.enabled, .radius_px = layout.dpi_scale(4)}),
                       A_ALARM_TOGGLE + i);
            y = row_bottom;
        }
    }
}

inline Scene build_main_scene(const Layout& layout, int client_w, const MainSceneState& state, const UiMakers& ui) {
    Scene scene;
    add_fill(scene, {0, 0, client_w, main_scene_height(layout, state)}, ui.surface());
    add_toolbar(scene, layout, client_w, state, ui);
    int y = layout.bar_h;
    add_clock(scene, layout, client_w, y, state, ui);
    add_stopwatch(scene, layout, client_w, y, state, ui);
    if (state.show_timers) {
        for (int i = 0; i < (int)state.timers.size(); ++i)
            add_timer(scene, layout, client_w, y, i, state.timers[i], ui);
    }
    add_alarms(scene, layout, client_w, y, state, ui);
    return scene;
}

inline bool contains(const RectI& rect, int x, int y) {
    return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
}

inline int hit_test(const Scene& scene, int x, int y) {
    for (auto it = scene.ops.rbegin(); it != scene.ops.rend(); ++it) {
        if (it->id != 0 && contains(it->rect, x, y)) return it->id;
    }
    return 0;
}

inline MainSceneState main_scene_state_from_app(const App& app, std::chrono::steady_clock::time_point now,
                                                std::string clock_text) {
    using namespace std::chrono;
    MainSceneState state{
        .topmost = app.topmost,
        .show_clock = app.show_clk,
        .show_stopwatch = app.show_sw,
        .show_timers = app.show_tmr,
        .show_alarms = app.show_alarms,
        .stopwatch_running = app.sw.is_running(),
        .clock_view = app.clock_view,
        .clock_text = std::move(clock_text),
        .stopwatch_text = wide_to_utf8(format_stopwatch_display(app.sw.elapsed(now))),
        .timers = {},
        .alarms = {},
    };

    for (const auto& alarm : app.alarms) {
        char buf[8] = {};
        std::snprintf(buf, sizeof(buf), "%02d:%02d", alarm.hour, alarm.minute);
        state.alarms.push_back(AlarmSceneState{
            .label = alarm.name,
            .time_text = buf,
            .enabled = alarm.enabled,
        });
    }

    for (int i = 0; i < (int)app.timers.size(); ++i) {
        const auto& timer = app.timers[i];
        bool expired = timer.t.expired(now);
        auto shown = timer.t.touched() ? timer.t.remaining(now) : timer.dur;
        double progress = 0.0;
        auto total = duration_cast<microseconds>(timer.dur).count();
        auto rem = duration_cast<microseconds>(timer.t.remaining(now)).count();
        if (timer.t.touched() && total > 0) progress = std::clamp((double)(total - rem) / (double)total, 0.0, 1.0);
        auto label = timer.label.empty() ? std::string{"Timer "} + std::to_string(i + 1) : wide_to_utf8(timer.label);
        state.timers.push_back(TimerSceneState{
            .running = timer.t.is_running(),
            .expired = expired,
            .progress = expired ? 1.0 : progress,
            .text = wide_to_utf8(format_timer_display(shown)),
            .label = std::move(label),
        });
    }
    if (state.timers.empty()) state.timers.push_back({});
    return state;
}

} // namespace ui_scene
