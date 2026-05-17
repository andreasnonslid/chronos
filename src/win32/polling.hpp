#pragma once
#include <windows.h>
#include "wndstate.hpp"

// ─── Redraw cadence ──────────────────────────────────────────────────────────
//
// Chronos repaints on two triggers:
//
//   1. Input events (key, mouse, focus, dialog return). These call
//      InvalidateRect directly — immediate, no polling involved.
//
//   2. Time passing.  WM_TIMER fires at the cadence that the *fastest*
//      currently-visible widget needs.  desired_poll_ms() picks that
//      cadence from the App state; sync_timer() arms or kills the timer.
//
// Picking the rate widget-by-widget means we never burn CPU drawing
// millisecond updates when the screen only shows minutes.  The rule of
// thumb: each widget contributes the longest interval at which a repaint
// will still look correct to a human eye.

constexpr int POLL_STOPWATCH_MS = 20;    // sw shows .{ms}, ~50 fps so digits don't tear
constexpr int POLL_TIMER_MS     = 100;   // countdown shows MM:SS; 10 fps keeps the second-flip crisp
constexpr int POLL_CLOCK_MS     = 1000;  // wall clock + alarm minute-flip detection; 1 Hz is plenty
constexpr int POLL_OFF          = 0;     // nothing time-driven on screen — stop the timer entirely

// Longest interval at which a repaint will still match reality, given the
// widgets currently visible/running. POLL_OFF means "don't tick at all"
// and is only safe when the window is minimized to the tray — visible
// windows always need at least POLL_CLOCK_MS for the title-bar wall clock
// and for clearing transient states (button blink, "Copied" title).
// The next input event will re-arm us via sync_timer.
int desired_poll_ms(const WndState& s);

void sync_timer(HWND hwnd, WndState& s);
void update_title(HWND hwnd, WndState& s);
void check_alarms(HWND hwnd, WndState& s);
void handle_wm_timer(HWND hwnd, WndState& s);
