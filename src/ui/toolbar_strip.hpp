#pragma once

// Pure function: given current state and this-frame input signals,
// returns the next value of toolbar_strip_open.
//
//   btn_clicked   – Button() returned true (hamburger clicked)
//   btn_hovered   – IsItemHovered() true immediately after the button
//   strip_hovered – IsWindowHovered() true inside the strip window last frame
//
// Rule priority:
//   1. click  → toggle (always wins)
//   2. button hover → open
//   3. strip open + nothing hovered → close
//   4. no change
inline bool toolbar_strip_next_state(bool currently_open,
                                     bool btn_clicked,
                                     bool btn_hovered,
                                     bool strip_hovered) noexcept {
    if (btn_clicked)                      return !currently_open;
    if (btn_hovered)                      return true;
    if (currently_open && !strip_hovered) return false;
    return currently_open;
}
