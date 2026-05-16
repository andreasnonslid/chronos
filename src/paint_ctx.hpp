#pragma once
#include <windows.h>
#include <chrono>
#include <optional>
#include <vector>
#include "app.hpp"
#include "gdi.hpp"
#include "layout.hpp"
#include "theme.hpp"
#include "ui_windows_painter.hpp"

// ─── Paint context ─────────────────────────────────────────────────
struct PaintCtx {
    const App& app;
    Layout& layout;
    const Theme& theme;
    WndResources res;
    std::vector<std::pair<RECT, int>>& btns;
    std::chrono::steady_clock::time_point now;
    bool global_hotkey_ok = true;
};

// ─── Helpers ─────────────────────────────────────────────────────
inline RECT btn(HDC hdc, RECT r, bool active, const wchar_t* label, int id, PaintCtx& ctx,
                std::optional<COLORREF> override_col = std::nullopt) {
    auto& layout = ctx.layout;
    bool blinking = id && ctx.app.blink_act == id && (ctx.now - ctx.app.blink_t) < BLINK_DUR;
    std::optional<UiColor> override_ui;
    if (override_col.has_value()) override_ui = ui_color_from_colorref(*override_col);
    ButtonConfig button_config{
        .active = active,
        .blinking = blinking,
        .fill_override = override_ui,
        .radius_px = layout.dpi_scale(6),
    };
    WidgetPaint button = make_button(ctx.theme.palette, button_config);

    win_paint_button(hdc, r, label, nullptr, button);
    if (id) ctx.btns.push_back({r, id});
    return r;
}

inline void divider(HDC hdc, int y, int cw, const PaintCtx& ctx) {
    win_paint_divider(hdc, 0, cw, y, make_ui(ctx.theme.palette).divider());
}
