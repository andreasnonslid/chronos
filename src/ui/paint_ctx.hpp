#pragma once
#include <windows.h>
#include <chrono>
#include <utility>
#include <vector>
#include "app.hpp"
#include "gdi.hpp"
#include "layout.hpp"
#include "theme.hpp"

// Per-frame paint context passed from the Win32 message pump into paint_scene.
// Holds references to the App state, scaled Layout, active Theme, the cached
// GDI handles to use this frame, and an output list of (rect, action_id) hit
// boxes the renderer fills as it walks the Scene.
struct PaintCtx {
    const App& app;
    Layout& layout;
    const Theme& theme;
    WndResources res;
    std::vector<std::pair<RECT, int>>& btns;
    std::chrono::steady_clock::time_point now;
    bool global_hotkey_ok = true;
};
