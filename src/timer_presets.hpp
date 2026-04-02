#pragma once

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
