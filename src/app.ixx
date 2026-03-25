module;
#include <chrono>
#include <string>
#include <vector>
export module app;
import stopwatch;
import timer;

export struct TimerSlot {
    Timer t;
    std::chrono::seconds dur{std::chrono::seconds{60}};
    bool notified = false;
    std::wstring label;
};

export struct App {
    Stopwatch sw;
    std::wstring sw_lap_file;
    std::vector<TimerSlot> timers;
    bool show_clk = true;
    bool show_sw = true;
    bool show_tmr = true;
    bool topmost = false;
    bool show_help = false;
    bool lap_write_failed = false;
    int blink_act = 0;
    std::chrono::steady_clock::time_point blink_t{};
    App() {
        timers.resize(1);
        for (auto& ts : timers) ts.t.set(ts.dur);
    }
};
