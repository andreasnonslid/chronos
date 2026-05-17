#pragma once
#include <format>
#include <string>
#include "assert.hpp"

constexpr int POMODORO_DEFAULT_CADENCE    = 4;
constexpr int POMODORO_MIN_CADENCE        = 1;
constexpr int POMODORO_MAX_CADENCE        = 10;
constexpr int POMODORO_WORK_SECS          = 25 * 60;
constexpr int POMODORO_SHORT_BREAK_SECS   = 5 * 60;
constexpr int POMODORO_LONG_BREAK_SECS    = 15 * 60;

inline constexpr int pomodoro_phase_count(int cadence) { return 2 * cadence; }

inline bool pomodoro_is_work(int phase) { return phase % 2 == 0; }

inline bool pomodoro_is_long_break(int phase, int cadence) {
    return phase == pomodoro_phase_count(cadence) - 1;
}

inline int pomodoro_next_phase(int phase, int cadence = POMODORO_DEFAULT_CADENCE) {
    return (phase + 1) % pomodoro_phase_count(cadence);
}

inline int pomodoro_phase_secs(int phase,
                               int work_secs  = POMODORO_WORK_SECS,
                               int short_secs = POMODORO_SHORT_BREAK_SECS,
                               int long_secs  = POMODORO_LONG_BREAK_SECS,
                               int cadence    = POMODORO_DEFAULT_CADENCE) {
    CHRONOS_ASSERT(phase >= 0 && phase < pomodoro_phase_count(cadence));
    if (pomodoro_is_work(phase)) return work_secs;
    if (pomodoro_is_long_break(phase, cadence)) return long_secs;
    return short_secs;
}

inline std::wstring pomodoro_phase_label(int phase, int cadence = POMODORO_DEFAULT_CADENCE) {
    CHRONOS_ASSERT(phase >= 0 && phase < pomodoro_phase_count(cadence));
    if (pomodoro_is_work(phase)) return std::format(L"Work {}/{}", phase / 2 + 1, cadence);
    if (pomodoro_is_long_break(phase, cadence)) return L"Long Break";
    return L"Short Break";
}
