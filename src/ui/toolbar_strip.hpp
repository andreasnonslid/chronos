#pragma once

// Two pure functions that together implement the hover-open / mouse-leave-close
// behaviour for the toolbar strip, split across the two render sites so that
// every ImGui hover query is read in the same frame it is used.
//
// render_titlebar()  calls toolbar_strip_btn_update()  — handles click + hover-open.
// render_toolbar_strip() calls toolbar_strip_should_stay_open() — handles close.
//
// Splitting at the render-site boundary is what makes both queries current-frame:
// render_titlebar stores ui.hamburger_btn_hovered, then render_toolbar_strip reads
// that value alongside its own IsWindowHovered() in the same frame.

// Applied in render_titlebar() after Button() + IsItemHovered().
// Handles click-toggle and hover-open; never closes (that is handled where
// current-frame strip hover is available).
inline bool toolbar_strip_btn_update(bool currently_open,
                                     bool btn_clicked,
                                     bool btn_hovered) noexcept {
    if (btn_clicked) return !currently_open;
    if (btn_hovered) return true;
    return currently_open;
}

// Applied inside render_toolbar_strip()'s Begin/End block, where both
// ui.hamburger_btn_hovered (written this frame by render_titlebar) and
// ImGui::IsWindowHovered() are current for the same frame.
// Returns false → caller should set toolbar_strip_open = false.
inline bool toolbar_strip_should_stay_open(bool btn_hovered,
                                           bool strip_hovered) noexcept {
    return btn_hovered || strip_hovered;
}
