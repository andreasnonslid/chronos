#pragma once
#include <windows.h>
#include <chrono>
#include "paint_ctx.hpp"

int  paint_alarms(HDC hdc, int cw, int y, PaintCtx& ctx);
void paint_help(HDC hdc, int cw, int y_bottom, PaintCtx& ctx);
void paint_all(HDC hdc, int cw, int ch, PaintCtx& ctx);
