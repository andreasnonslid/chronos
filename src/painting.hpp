#pragma once
#include <windows.h>
#include <chrono>
#include "paint_ctx.hpp"

int  paint_bar(HDC hdc, int cw, PaintCtx& ctx);
int  paint_clock(HDC hdc, int cw, int y, PaintCtx& ctx);
int  paint_stopwatch(HDC hdc, int cw, int y, PaintCtx& ctx,
                     std::chrono::steady_clock::time_point now);
int  paint_alarms(HDC hdc, int cw, int y, PaintCtx& ctx);
void paint_help(HDC hdc, int cw, int y_bottom, PaintCtx& ctx);
void paint_all(HDC hdc, int cw, int ch, PaintCtx& ctx);
