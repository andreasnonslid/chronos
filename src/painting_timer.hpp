#pragma once
#include <windows.h>
#include <chrono>
#include "paint_ctx.hpp"

int paint_timers(HDC hdc, int cw, int y, PaintCtx& ctx,
                 std::chrono::steady_clock::time_point now);
