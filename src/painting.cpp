#include "painting.hpp"
#include <windows.h>
#include <chrono>
#include "actions.hpp"
#include "app.hpp"
#include "layout.hpp"
#include "paint_ctx.hpp"
#include "painting_scene.hpp"
#include "theme.hpp"
#include "ui_scene.hpp"

void paint_all(HDC hdc, int cw, int ch, PaintCtx& ctx) {
    ctx.btns.clear();
    SetBkMode(hdc, TRANSPARENT);

    RECT all{0, 0, cw, ch};
    FillRect(hdc, &all, ctx.res.brBg);

    ctx.now = std::chrono::steady_clock::now();

    SYSTEMTIME st;
    GetLocalTime(&st);
    UiMakers ui = make_ui(ctx.theme.palette);
    auto scene_state = ui_scene::main_scene_state_from_app(ctx.app, ctx.now, st.wHour, st.wMinute, st.wSecond,
                                                           ctx.global_hotkey_ok);
    auto scene = ui_scene::build_main_scene(ctx.layout, cw, scene_state, ui);
    paint_scene(hdc, scene, ctx);
}
