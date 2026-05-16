#pragma once
#include <chrono>
#include <string>
#include "config.hpp"

// Renders the wall-clock readout for the given ClockView. For Analog, returns
// the H24_HMS string (Scene consumers use it as a fallback; the analog face
// itself is rendered separately).
std::string format_clock_text(ClockView view, int hour24, int minute, int second);

std::wstring format_stopwatch_short(std::chrono::steady_clock::duration d);
std::wstring format_stopwatch_long(std::chrono::steady_clock::duration d);
std::wstring format_stopwatch_display(std::chrono::steady_clock::duration d);
std::wstring format_timer_display(std::chrono::steady_clock::duration d);
std::wstring format_timer_edit(std::chrono::steady_clock::duration d);
std::wstring format_worked_time(std::chrono::seconds s);
std::wstring format_lap_row(std::size_t lap_number, std::chrono::steady_clock::duration split,
                             std::chrono::steady_clock::duration total);
std::wstring format_timer_title(const std::wstring& label, int index, int total,
                                 const std::wstring& time_str, bool expired);
std::wstring format_tray_title(int index, int total);
