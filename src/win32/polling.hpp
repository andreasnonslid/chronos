#pragma once
#include <windows.h>
#include "wndstate.hpp"

constexpr int POLL_STOPWATCH_MS = 20;
constexpr int POLL_TIMER_MS    = 100;
constexpr int POLL_IDLE_MS     = 1000;

void sync_timer(HWND hwnd, WndState& s);
void update_title(HWND hwnd, WndState& s);
void check_alarms(HWND hwnd, WndState& s);
void handle_wm_timer(HWND hwnd, WndState& s);
