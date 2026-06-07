#pragma once

// Pure functions for the two render sites that together control toolbar_strip_open.
//
// render_titlebar()      calls toolbar_strip_btn_update()   — click/hover open.
// render_toolbar_strip() calls toolbar_strip_click_outside() — close on click outside.
//
// Design: the strip behaves like a standard dropdown.
//   • Click hamburger → toggle (open/close).
//   • Hover hamburger → open (if closed).
//   • Click anywhere outside the strip → close.
//   • Click an action button inside the strip → close (handled by caller).
// No "hover-leave closes" — that caused the strip to close the instant the user
// moved the mouse from the button toward the strip buttons.

// Applied in render_titlebar() after Button() + IsItemHovered().
// Handles click-toggle and hover-open; never closes.
inline bool toolbar_strip_btn_update(bool currently_open,
                                     bool btn_clicked,
                                     bool btn_hovered) noexcept {
    if (btn_clicked) return !currently_open;
    if (btn_hovered) return true;
    return currently_open;
}

// Applied inside render_toolbar_strip()'s Begin/End block.
// Returns true when a left-click occurred outside both the strip window and the
// hamburger button, meaning the strip should close.
//
//   left_click            — ImGui::IsMouseClicked(ImGuiMouseButton_Left)
//   window_or_btn_hovered — IsWindowHovered() || ui.hamburger_btn_hovered
inline bool toolbar_strip_click_outside(bool left_click,
                                        bool window_or_btn_hovered) noexcept {
    return left_click && !window_or_btn_hovered;
}
