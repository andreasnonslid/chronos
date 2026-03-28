#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <chrono>
#include <optional>
#include <vector>
#include "app.hpp"
#include "gdi.hpp"
#include "layout.hpp"
#include "theme.hpp"

// ─── Paint context ─────────────────────────────────────────────────
struct PaintCtx {
    App& app;
    Layout& layout;
    const Theme& theme;
    const WndResources& res;
    std::vector<std::pair<RECT, int>>& btns;
    std::chrono::steady_clock::time_point now;
};

// ─── Helpers ─────────────────────────────────────────────────────
inline RECT btn(HDC hdc, RECT r, bool active, const wchar_t* label, int id, PaintCtx& ctx,
                std::optional<COLORREF> override_col = std::nullopt) {
    auto& layout = ctx.layout;
    bool blinking = id && ctx.app.blink_act == id && (ctx.now - ctx.app.blink_t) < BLINK_DUR;
    HBRUSH brush;
    GdiObj dyn_br{nullptr};
    if (blinking) {
        brush = ctx.res.brBlink;
    } else if (override_col.has_value()) {
        dyn_br.h = CreateSolidBrush(*override_col);
        brush = (HBRUSH)dyn_br.h;
    } else {
        brush = active ? ctx.res.brActive : ctx.res.brBtn;
    }
    auto* obr = (HBRUSH)SelectObject(hdc, brush);
    auto* opn = (HPEN)SelectObject(hdc, ctx.res.pnNull);
    int rr = layout.dpi_scale(6);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rr, rr);
    SelectObject(hdc, obr);
    SelectObject(hdc, opn);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ctx.theme.text);
    DrawTextW(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (id) ctx.btns.push_back({r, id});
    return r;
}

inline void divider(HDC hdc, int y, int cw, const PaintCtx& ctx) {
    auto* op = (HPEN)SelectObject(hdc, ctx.res.pnDivider);
    MoveToEx(hdc, 0, y, nullptr);
    LineTo(hdc, cw, y);
    SelectObject(hdc, op);
}
