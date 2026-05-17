#pragma once
#include <format>
#include <string>

struct TimerPreset {
    int secs;
    const wchar_t* label;
};

inline constexpr TimerPreset TIMER_PRESETS[] = {
    {   60, L"1:00"},     {  120, L"2:00"},   {  180, L"3:00"},
    {  300, L"5:00"},     {  600, L"10:00"},  {  900, L"15:00"},
    { 1200, L"20:00"},    { 1500, L"25:00"},  { 1800, L"30:00"},
    { 2700, L"45:00"},    { 3600, L"1:00:00"},
};

inline std::wstring format_preset_label(int secs) {
    int h = secs / 3600;
    int m = (secs / 60) % 60;
    int s = secs % 60;
    if (h > 0)
        return s > 0 ? std::format(L"{}:{:02}:{:02}", h, m, s)
                     : std::format(L"{}:{:02}:00", h, m);
    return s > 0 ? std::format(L"{}:{:02}", m, s) : std::format(L"{}:00", m);
}
