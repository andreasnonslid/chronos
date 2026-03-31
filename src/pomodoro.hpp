#pragma once
#include <format>
#include <string>

// Pomodoro cycle: Work(25m) · Break(5m) × 4, then LongBreak(15m)
// Phases 0,2,4,6 = work; 1,3,5 = short break; 7 = long break
constexpr int POMODORO_WORK_SECS        = 25 * 60;
constexpr int POMODORO_SHORT_BREAK_SECS = 5 * 60;
constexpr int POMODORO_LONG_BREAK_SECS  = 15 * 60;

inline int pomodoro_phase_secs(int phase) {
    int p = phase % 8;
    if (p % 2 == 0) return POMODORO_WORK_SECS;
    if (p == 7) return POMODORO_LONG_BREAK_SECS;
    return POMODORO_SHORT_BREAK_SECS;
}

inline std::wstring pomodoro_phase_label(int phase) {
    int p = phase % 8;
    if (p % 2 == 0) return std::format(L"Work {}/4", p / 2 + 1);
    if (p == 7) return L"Long Break";
    return L"Short Break";
}
