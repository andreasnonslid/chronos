#pragma once
// Shared test utilities. Keep this header thin: only zero-argument-or-trivial
// helpers that several test files would otherwise redefine. Anything specific
// to one test file belongs in that file.

#include <chrono>
#include "app.hpp"
#include "config.hpp"

namespace test_helpers {

using sc = std::chrono::steady_clock;

inline sc::time_point t0() { return sc::time_point{}; }

inline sc::time_point at_ms(int ms) {
    return t0() + std::chrono::milliseconds(ms);
}

inline sc::time_point at_s(int s) {
    return t0() + std::chrono::seconds(s);
}

// Set a timer slot's duration and reset the underlying Timer to a fresh state
// armed for `dur`. Used by tests that need to seed a non-default timer length
// before dispatching adjust/start actions.
inline void set_timer_dur(App& app, int idx, std::chrono::seconds dur) {
    app.timers[idx].dur = dur;
    app.timers[idx].t.reset();
    app.timers[idx].t.set(dur);
}

} // namespace test_helpers
