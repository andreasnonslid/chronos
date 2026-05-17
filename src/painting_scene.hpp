#pragma once
#include <windows.h>
#include "paint_ctx.hpp"
#include "ui_scene.hpp"

// Render a platform-neutral ui_scene::Scene onto a Windows HDC. Each Op is
// translated to the matching win_paint_* primitive; Ops carrying a non-zero
// id are appended to ctx.btns so input hit-testing keeps working.
//
// Text Ops currently render with the toolbar font (fontSm). A future pass will
// extend ui_scene::Op with a font/style hint as more widgets migrate.
void paint_scene(HDC hdc, const ui_scene::Scene& scene, PaintCtx& ctx);
