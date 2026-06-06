#pragma once
// Internal header shared by all ui_*.cpp translation units.
// Not part of the public API — do not include from outside src/platform/.
#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
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
#include "ui_app.hpp"
#include "ui_style.hpp"

// ─── Platform time helper ─────────────────────────────────────────────────────

inline tm local_tm(time_t t) {
    tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    return lt;
}

// ─── Color / string helpers ───────────────────────────────────────────────────

inline ImVec4 to_v4(UiColor c, float a = 1.f) {
    return {c.r / 255.f, c.g / 255.f, c.b / 255.f, a};
}
inline ImU32 to_u32(UiColor c, float a = 1.f) {
    return IM_COL32(c.r, c.g, c.b, (int)(a * 255));
}
inline std::string ws(const std::wstring& w) { return wide_to_utf8(w); }

// ─── Debug overlay ────────────────────────────────────────────────────────────

#ifdef CHRONOS_DEBUG_UI_OVERLAY
extern const bool* debug_flag;  // defined in ui_app.cpp
void debug_last_item(const char* label, ImU32 color = IM_COL32(255, 255, 255, 230));
#define CHRONOS_DEBUG_ITEM(label, color) debug_last_item(label, color)
#else
#define CHRONOS_DEBUG_ITEM(label, color) ((void)0)
#endif

// ─── Render function declarations ─────────────────────────────────────────────

void render_titlebar(App& app, UiState& ui, const ThemePalette& pal);
void render_clock(App& app, UiState& ui, const ThemePalette& pal, float preferred_height);
void render_stopwatch(App& app, UiState& ui, const ThemePalette& pal);
void render_timers(App& app, UiState& ui, const ThemePalette& pal);
void render_alarms(App& app, UiState& ui);
void render_add_alarm_window(App& app, UiState& ui);
void render_settings_window(App& app, UiState& ui);
