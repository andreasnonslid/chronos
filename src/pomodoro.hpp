#pragma once
#include <format>
#include <string>
#include "assert.hpp"

// Pomodoro cycle: Work(25m) · Break(5m) × 4, then LongBreak(15m)
enum class PomodoroPhase : int {
    Work1       = 0,
    ShortBreak1 = 1,
    Work2       = 2,
    ShortBreak2 = 3,
    Work3       = 4,
    ShortBreak3 = 5,
    Work4       = 6,
    LongBreak   = 7,
};

constexpr int POMODORO_PHASE_COUNT      = 8;
constexpr int POMODORO_WORK_SECS        = 25 * 60;
constexpr int POMODORO_SHORT_BREAK_SECS = 5 * 60;
constexpr int POMODORO_LONG_BREAK_SECS  = 15 * 60;

static_assert(int(PomodoroPhase::LongBreak) == POMODORO_PHASE_COUNT - 1,
              "POMODORO_PHASE_COUNT out of sync with PomodoroPhase enum");

inline bool pomodoro_is_work(PomodoroPhase p) { return int(p) % 2 == 0; }

inline PomodoroPhase pomodoro_next_phase(PomodoroPhase p) {
    return static_cast<PomodoroPhase>((int(p) + 1) % POMODORO_PHASE_COUNT);
}

inline int pomodoro_phase_secs(PomodoroPhase phase,
                               int work_secs  = POMODORO_WORK_SECS,
                               int short_secs = POMODORO_SHORT_BREAK_SECS,
                               int long_secs  = POMODORO_LONG_BREAK_SECS) {
    CHRONOS_ASSERT(int(phase) >= 0 && int(phase) < POMODORO_PHASE_COUNT);
    if (pomodoro_is_work(phase)) return work_secs;
    if (phase == PomodoroPhase::LongBreak) return long_secs;
    return short_secs;
}

inline std::wstring pomodoro_phase_label(PomodoroPhase phase) {
    CHRONOS_ASSERT(int(phase) >= 0 && int(phase) < POMODORO_PHASE_COUNT);
    if (pomodoro_is_work(phase)) return std::format(L"Work {}/4", int(phase) / 2 + 1);
    if (phase == PomodoroPhase::LongBreak) return L"Long Break";
    return L"Short Break";
}
