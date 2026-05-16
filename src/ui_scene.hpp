#pragma once
#include <algorithm>
#include <chrono>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "actions.hpp"
#include "app.hpp"
#include "encoding.hpp"
#include "formatting.hpp"
#include "layout.hpp"
#include "pomodoro.hpp"
#include "ui_style.hpp"

namespace ui_scene {

struct RectI {
    int left;
    int top;
    int right;
    int bottom;
};

enum class Align { Left, Center, Right };

// Logical text style — backends pick a concrete font. Small is the default
// (matches the toolbar / labels); Large/Big are used for the timer running
// readout and the digital clock respectively.
enum class TextStyle { Small, Large, Big };

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
    TextStyle text_style = TextStyle::Small;
    int id = 0;
};

struct Scene {
    std::vector<Op> ops;
};

struct TimerSceneState {
    // Idle (untouched) timers show editable HH/MM/SS columns with adjust arrows.
    // Touched timers show a single running readout. Pomodoro mode replaces the
    // editable columns with a single duration string and adds a "Skip" button.
    bool touched = false;            // false = idle/edit mode
    bool running = false;
    bool expired = false;
    bool pomodoro = false;
    double progress = 0.0;
    std::string readout = "01:00";   // running/expired text or idle pomodoro duration
    TextTone readout_tone = TextTone::Primary;
    std::string subtitle;            // dim line above readout (label, edit string, or phase)
    // Idle non-pomodoro: per-column text and adjust-arrow action ids.
    std::string hh_text;
    std::string mm_text;
    std::string ss_text;
    bool can_add = false;            // show "+" to spawn another slot
    bool can_remove = false;         // show "-" to drop this slot
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
    bool stopwatch_has_lap_file = false;
    bool stopwatch_lap_write_failed = false;
    ClockView clock_view = ClockView::H24_HMS;
    std::string clock_text = "00:00:00";
    std::string stopwatch_text = "00:00.000";
    // Single-line lap summary shown below the stopwatch buttons.
    // Empty string is rendered as a dim em-dash placeholder.
    std::string stopwatch_lap_info;
    // Action id of the button currently flashing as click feedback (0 = none).
    int blink_act = 0;
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
                     int id = 0, TextStyle style = TextStyle::Small) {
    Op op{};
    op.kind = OpKind::Text;
    op.rect = rect;
    op.text = std::move(text);
    op.text_color = paint.color;
    op.align = align;
    op.text_style = style;
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

// Convenience overload: derives the WidgetPaint from a ButtonConfig and the
// scene's blink hint. Lets callers stop repeating the
// "state.blink_act != 0 && state.blink_act == action" pattern at every button.
inline void add_action_button(Scene& scene, const UiMakers& ui, RectI rect, std::string label,
                              ButtonConfig config, int action, int blink_act = 0) {
    if (action != 0 && action == blink_act) config.blinking = true;
    add_button(scene, rect, std::move(label), ui.button(config), action);
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
        {layout.w_set, "⚙", false, A_SETTINGS}, // U+2699 gear
    };

    int total_w = (static_cast<int>(std::size(buttons)) - 1) * layout.bar_gap;
    for (const auto& b : buttons) total_w += b.width;
    int x = (client_w - total_w) / 2;
    int y = (layout.bar_h - layout.btn_h) / 2;
    for (const auto& b : buttons) {
        add_action_button(scene, ui, {x, y, x + b.width, y + layout.btn_h}, b.label,
                          ButtonConfig{.active = b.active, .radius_px = layout.dpi_scale(6)},
                          b.action, state.blink_act);
        x += b.width + layout.bar_gap;
    }
}

inline void add_clock(Scene& scene, const Layout& layout, int client_w, int& y, const MainSceneState& state,
                      const UiMakers& ui) {
    if (!state.show_clock) return;
    int h = effective_clk_h(layout, state.clock_view);
    add_divider(scene, 0, client_w, y, ui.divider());
    add_text(scene, {0, y, client_w, y + h}, state.clock_text, ui.text(), Align::Center, A_CLK_CYCLE, TextStyle::Big);
    y += h;
}

inline void add_stopwatch(Scene& scene, const Layout& layout, int client_w, int& y, const MainSceneState& state,
                          const UiMakers& ui) {
    if (!state.show_stopwatch) return;
    add_divider(scene, 0, client_w, y, ui.divider());
    add_text(scene, {0, y + layout.dpi_scale(4), client_w, y + layout.dpi_scale(44)}, state.stopwatch_text, ui.text(),
             Align::Center, 0, TextStyle::Big);

    int bw = layout.dpi_scale(76);
    int gap = layout.dpi_scale(6);
    int bh = layout.dpi_scale(28);
    int x0 = (client_w - 3 * bw - 2 * gap) / 2;
    int by = y + layout.dpi_scale(46);
    int radius = layout.dpi_scale(6);
    add_action_button(scene, ui, {x0, by, x0 + bw, by + bh}, state.stopwatch_running ? "Stop" : "Start",
                      ButtonConfig{.active = state.stopwatch_running, .radius_px = radius}, A_SW_START,
                      state.blink_act);
    add_action_button(scene, ui, {x0 + bw + gap, by, x0 + 2 * bw + gap, by + bh}, "Lap",
                      ButtonConfig{.radius_px = radius}, A_SW_LAP, state.blink_act);
    add_action_button(scene, ui, {x0 + 2 * (bw + gap), by, x0 + 3 * bw + 2 * gap, by + bh}, "Reset",
                      ButtonConfig{.radius_px = radius}, A_SW_RESET, state.blink_act);

    // Lap info row below the buttons (em-dash placeholder when empty).
    add_text(scene,
             {0, by + bh + layout.dpi_scale(4), client_w, by + bh + layout.dpi_scale(22)},
             state.stopwatch_lap_info.empty() ? "—" : state.stopwatch_lap_info,
             ui.text(TextConfig{.tone = TextTone::Dim}), Align::Center);

    // "Get Laps" button: fill expresses state (failed/enabled/disabled);
    // action id is 0 when no lap file exists so input ignores the click.
    int gbw = layout.dpi_scale(100);
    int gbh = layout.dpi_scale(18);
    UiColor lap_fill = state.stopwatch_lap_write_failed ? ui.palette.expire
                       : state.stopwatch_has_lap_file   ? ui.palette.btn
                                                        : ui.palette.dim;
    const char* lap_label = state.stopwatch_lap_write_failed ? "Get Laps (!)" : "Get Laps";
    add_action_button(scene, ui,
                      {(client_w - gbw) / 2, y + layout.sw_h - gbh, (client_w + gbw) / 2, y + layout.sw_h},
                      lap_label, ButtonConfig{.fill_override = lap_fill, .radius_px = radius},
                      state.stopwatch_has_lap_file ? A_SW_GET : 0, state.blink_act);

    y += layout.sw_h;
}

inline void add_timer(Scene& scene, const Layout& layout, int client_w, int& y, int index, const TimerSceneState& timer,
                      const UiMakers& ui, int blink_act = 0) {
    add_divider(scene, 0, client_w, y, ui.divider());

    // Progress fill (only when the timer has been touched at least once).
    if (timer.touched) {
        int progress_w = std::clamp((int)(client_w * timer.progress), 0, client_w);
        if (progress_w > 0 || timer.expired) {
            add_progress(scene, {0, y, timer.expired ? client_w : progress_w, y + layout.tmr_h},
                         ui.progress(ProgressConfig{.expired = timer.expired}));
        }
    }

    auto metrics = TimerMetrics::from(layout);
    int hh_cx = client_w / 2 - metrics.col_gap;
    int mm_cx = client_w / 2;
    int ss_cx = client_w / 2 + metrics.col_gap;
    int radius = layout.dpi_scale(6);

    auto emit_arrows = [&](int y_off, const char* glyph, int hh_act, int mm_act, int ss_act) {
        for (auto [cx, act] : {std::pair{hh_cx, hh_act}, {mm_cx, mm_act}, {ss_cx, ss_act}}) {
            add_action_button(scene, ui,
                              {cx - metrics.abw / 2, y + y_off, cx + metrics.abw / 2, y + y_off + metrics.abh},
                              glyph, ButtonConfig{.radius_px = radius}, tmr_act(index, act), blink_act);
        }
    };

    // Subtitle: dim row above the big readout. Used by running mode and
    // pomodoro idle mode; idle non-pomodoro shows the label here if any.
    if (!timer.subtitle.empty()) {
        add_text(scene, {0, y + metrics.up_off, client_w, y + metrics.up_off + layout.dpi_scale(20)},
                 timer.subtitle, ui.text(TextConfig{.tone = TextTone::Dim}), Align::Center);
    }

    if (!timer.touched && !timer.pomodoro) {
        // Idle non-pomodoro: three editable columns + arrow buttons.
        emit_arrows(metrics.up_off, "▲", A_TMR_HUP, A_TMR_MUP, A_TMR_SUP);
        int field_half = layout.dpi_scale(22);
        int field_top = y + metrics.td_off;
        int field_bottom = field_top + layout.dpi_scale(40);
        struct Field { int cx; const std::string& text; };
        Field fields[] = {{hh_cx, timer.hh_text}, {mm_cx, timer.mm_text}, {ss_cx, timer.ss_text}};
        for (const auto& f : fields)
            add_text(scene, {f.cx - field_half, field_top, f.cx + field_half, field_bottom},
                     f.text, ui.text(), Align::Center, 0, TextStyle::Big);
        int sep_w = layout.dpi_scale(8);
        int sep1_cx = (hh_cx + mm_cx) / 2;
        int sep2_cx = (mm_cx + ss_cx) / 2;
        add_text(scene, {sep1_cx - sep_w / 2, field_top, sep1_cx + sep_w / 2, field_bottom}, ":",
                 ui.text(), Align::Center, 0, TextStyle::Big);
        add_text(scene, {sep2_cx - sep_w / 2, field_top, sep2_cx + sep_w / 2, field_bottom}, ":",
                 ui.text(), Align::Center, 0, TextStyle::Big);
        emit_arrows(metrics.dn_off, "▼", A_TMR_HDN, A_TMR_MDN, A_TMR_SDN);
    } else {
        // Running OR pomodoro-idle: single big readout below the subtitle.
        add_text(scene,
                 {0, y + metrics.up_off + layout.dpi_scale(20), client_w, y + metrics.dn_off + metrics.abh},
                 timer.readout, ui.text(TextConfig{.tone = timer.readout_tone}), Align::Center, 0, TextStyle::Large);
    }

    // Control buttons row.
    int gap = layout.dpi_scale(6);
    int bh = layout.dpi_scale(28);
    int by = y + layout.tmr_h - bh;
    if (timer.pomodoro) {
        int cw3 = layout.dpi_scale(58);
        int cx0 = (client_w - 3 * cw3 - 2 * gap) / 2;
        add_action_button(scene, ui, {cx0, by, cx0 + cw3, by + bh}, timer.running ? "Pause" : "Start",
                          ButtonConfig{.active = timer.running, .radius_px = radius},
                          tmr_act(index, A_TMR_START), blink_act);
        add_action_button(scene, ui, {cx0 + cw3 + gap, by, cx0 + 2 * cw3 + gap, by + bh}, "Skip",
                          ButtonConfig{.radius_px = radius}, tmr_act(index, A_TMR_SKIP), blink_act);
        add_action_button(scene, ui, {cx0 + 2 * (cw3 + gap), by, cx0 + 3 * cw3 + 2 * gap, by + bh}, "Reset",
                          ButtonConfig{.radius_px = radius}, tmr_act(index, A_TMR_RST), blink_act);
    } else {
        int cw2 = layout.dpi_scale(86);
        int cx0 = (client_w - 2 * cw2 - gap) / 2;
        add_action_button(scene, ui, {cx0, by, cx0 + cw2, by + bh}, timer.running ? "Pause" : "Start",
                          ButtonConfig{.active = timer.running, .radius_px = radius},
                          tmr_act(index, A_TMR_START), blink_act);
        add_action_button(scene, ui, {cx0 + cw2 + gap, by, cx0 + 2 * cw2 + gap, by + bh}, "Reset",
                          ButtonConfig{.radius_px = radius}, tmr_act(index, A_TMR_RST), blink_act);
    }

    // Slot add/remove buttons in the corners.
    int pm_sz = layout.dpi_scale(22);
    int pm_margin = layout.dpi_scale(6);
    int pm_top = y + layout.tmr_h - pm_sz;
    if (timer.can_add)
        add_action_button(scene, ui, {client_w - pm_margin - pm_sz, pm_top, client_w - pm_margin, pm_top + pm_sz},
                          "+", ButtonConfig{.radius_px = radius}, tmr_act(index, A_TMR_ADD), blink_act);
    if (timer.can_remove)
        add_action_button(scene, ui, {pm_margin, pm_top, pm_margin + pm_sz, pm_top + pm_sz},
                          "−", ButtonConfig{.radius_px = radius}, tmr_act(index, A_TMR_DEL), blink_act);

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
    add_action_button(scene, ui,
                      {client_w - bw - layout.dpi_scale(4), y + layout.dpi_scale(7), client_w - layout.dpi_scale(4),
                       header_bottom - layout.dpi_scale(7)},
                      "Add", ButtonConfig{.radius_px = layout.dpi_scale(4)}, A_ALARM_ADD, state.blink_act);
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
            add_action_button(scene, ui,
                              {client_w - bw - layout.dpi_scale(4), y + layout.dpi_scale(4),
                               client_w - layout.dpi_scale(4), row_bottom - layout.dpi_scale(4)},
                              alarm.enabled ? "On" : "Off",
                              ButtonConfig{.active = alarm.enabled, .radius_px = layout.dpi_scale(4)},
                              A_ALARM_TOGGLE + i, state.blink_act);
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
            add_timer(scene, layout, client_w, y, i, state.timers[i], ui, state.blink_act);
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
    const auto& laps = app.sw.laps();
    std::string lap_info;
    if (!laps.empty()) {
        lap_info = "Lap " + std::to_string(laps.size()) + "  —  " +
                   wide_to_utf8(format_stopwatch_short(laps.back()));
    }
    MainSceneState state{
        .topmost = app.topmost,
        .show_clock = app.show_clk,
        .show_stopwatch = app.show_sw,
        .show_timers = app.show_tmr,
        .show_alarms = app.show_alarms,
        .stopwatch_running = app.sw.is_running(),
        .stopwatch_has_lap_file = !app.sw_lap_file.empty(),
        .stopwatch_lap_write_failed = app.lap_write_failed,
        .clock_view = app.clock_view,
        .clock_text = std::move(clock_text),
        .stopwatch_text = wide_to_utf8(format_stopwatch_display(app.sw.elapsed(now))),
        .stopwatch_lap_info = std::move(lap_info),
        // Mirror theme.hpp's BLINK_DUR (kept inline to keep this header platform-neutral).
        .blink_act = (app.blink_act != 0 && (now - app.blink_t) < std::chrono::milliseconds{120}) ? app.blink_act : 0,
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

    bool can_add = (int)app.timers.size() < Config::MAX_TIMERS;
    bool can_remove = (int)app.timers.size() > 1;
    for (int i = 0; i < (int)app.timers.size(); ++i) {
        const auto& timer = app.timers[i];
        bool touched = timer.t.touched();
        bool expired = timer.t.expired(now);
        auto remaining = timer.t.remaining(now);
        double progress = 0.0;
        auto total = duration_cast<microseconds>(timer.dur).count();
        auto rem = duration_cast<microseconds>(remaining).count();
        if (touched && total > 0) progress = std::clamp((double)(total - rem) / (double)total, 0.0, 1.0);

        TimerSceneState ts{};
        ts.touched = touched;
        ts.running = timer.t.is_running();
        ts.expired = expired;
        ts.pomodoro = timer.pomodoro;
        ts.progress = expired ? 1.0 : progress;
        ts.can_add = can_add;
        ts.can_remove = can_remove;

        // Decide subtitle and readout based on touched / pomodoro / running.
        if (touched) {
            // Running mode subtitle: label or the editable duration string.
            std::wstring subtitle = timer.label.empty()
                                        ? format_timer_edit(std::chrono::duration_cast<Timer::dur>(timer.dur))
                                        : timer.label;
            if (timer.pomodoro && timer.pomodoro_work_elapsed.count() > 0)
                subtitle += L" · " + format_worked_time(timer.pomodoro_work_elapsed);
            ts.subtitle = wide_to_utf8(subtitle);
            ts.readout = wide_to_utf8(format_timer_display(remaining));
            ts.readout_tone = expired                                            ? TextTone::Danger
                              : (remaining < std::chrono::seconds{10})           ? TextTone::Warning
                                                                                 : TextTone::Primary;
        } else if (timer.pomodoro) {
            // Idle pomodoro: phase label (+ worked) as subtitle, duration centered.
            std::wstring subtitle = pomodoro_phase_label(timer.pomodoro_phase, app.pomodoro_cadence);
            if (timer.pomodoro_work_elapsed.count() > 0)
                subtitle += L" · " + format_worked_time(timer.pomodoro_work_elapsed);
            ts.subtitle = wide_to_utf8(subtitle);
            ts.readout = wide_to_utf8(format_timer_display(timer.dur));
        } else {
            // Idle non-pomodoro: optional label as subtitle, HH/MM/SS columns.
            if (!timer.label.empty()) ts.subtitle = wide_to_utf8(timer.label);
            long long total_s = timer.dur.count();
            ts.hh_text = std::to_string(total_s / 3600);
            ts.mm_text = std::format("{:02}", (total_s / 60) % 60);
            ts.ss_text = std::format("{:02}", total_s % 60);
        }

        state.timers.push_back(std::move(ts));
    }
    if (state.timers.empty()) state.timers.push_back({});
    return state;
}

} // namespace ui_scene
