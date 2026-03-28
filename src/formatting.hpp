#pragma once
#include <chrono>
#include <format>
#include <string>

inline std::wstring format_stopwatch_short(std::chrono::steady_clock::duration d) {
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    auto ms = total_ms % 1000;
    auto total_s = total_ms / 1000;
    auto s = total_s % 60;
    auto m = total_s / 60;
    return std::format(L"{:02}:{:02}.{:03}", m, s, ms);
}

inline std::wstring format_stopwatch_long(std::chrono::steady_clock::duration d) {
    auto total_s = std::chrono::duration_cast<std::chrono::seconds>(d).count();
    auto s = total_s % 60;
    auto m = (total_s / 60) % 60;
    auto h = total_s / 3600;
    return std::format(L"{:02}:{:02}:{:02}", h, m, s);
}

inline std::wstring format_stopwatch_display(std::chrono::steady_clock::duration d) {
    return d >= std::chrono::hours{1} ? format_stopwatch_long(d) : format_stopwatch_short(d);
}

inline std::wstring format_timer_display(std::chrono::steady_clock::duration d) {
    auto total_s = std::chrono::duration_cast<std::chrono::seconds>(d).count();
    if (total_s >= 3600) {
        auto h = total_s / 3600;
        auto m = (total_s / 60) % 60;
        auto s = total_s % 60;
        return std::format(L"{:02}:{:02}:{:02}", h, m, s);
    }
    auto s = total_s % 60;
    auto m = total_s / 60;
    return std::format(L"{:02}:{:02}", m, s);
}

// Always shows H:MM:SS format for timer editing UI
inline std::wstring format_timer_edit(std::chrono::steady_clock::duration d) {
    auto total_s = std::chrono::duration_cast<std::chrono::seconds>(d).count();
    auto h = total_s / 3600;
    auto m = (total_s / 60) % 60;
    auto s = total_s % 60;
    return std::format(L"{}:{:02}:{:02}", h, m, s);
}

inline std::wstring format_lap_row(std::size_t lap_number, std::chrono::steady_clock::duration split,
                                    std::chrono::steady_clock::duration total) {
    return std::format(L"Lap {:<3}   split {:<14}   total {}", lap_number, format_stopwatch_short(split),
                       format_stopwatch_short(total));
}
