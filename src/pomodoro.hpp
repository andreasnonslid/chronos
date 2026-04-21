#pragma once
#include <format>
#include <string>
#include "assert.hpp"

// Pomodoro cycle: Work(25m) · Break(5m) × 4, then LongBreak(15m)
// Phases 0,2,4,6 = work; 1,3,5 = short break; 7 = long break
constexpr int POMODORO_PHASE_COUNT      = 8;
constexpr int POMODORO_WORK_SECS        = 25 * 60;
constexpr int POMODORO_SHORT_BREAK_SECS = 5 * 60;
constexpr int POMODORO_LONG_BREAK_SECS  = 15 * 60;

inline int pomodoro_phase_secs(int phase,
                               int work_secs  = POMODORO_WORK_SECS,
                               int short_secs = POMODORO_SHORT_BREAK_SECS,
                               int long_secs  = POMODORO_LONG_BREAK_SECS) {
    CHRONOS_ASSERT(phase >= 0 && phase < POMODORO_PHASE_COUNT);
    if (phase % 2 == 0) return work_secs;
    if (phase == 7) return long_secs;
    return short_secs;
}

inline std::wstring pomodoro_phase_label(int phase) {
    CHRONOS_ASSERT(phase >= 0 && phase < POMODORO_PHASE_COUNT);
    if (phase % 2 == 0) return std::format(L"Work {}/4", phase / 2 + 1);
    if (phase == 7) return L"Long Break";
    return L"Short Break";
}
