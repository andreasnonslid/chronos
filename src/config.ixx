module;
#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
export module config;

export struct Config {
    bool show_clk  = true;
    bool show_sw   = true;
    bool show_tmr  = true;
    bool topmost   = false;
    static constexpr int MAX_TIMERS    = 3;
    static constexpr int TIMER_MIN_SECS = 0;
    static constexpr int TIMER_MAX_SECS = 86400;
    static constexpr int MIN_WINDOW_W   = 260;
    int  num_timers = 1;
    std::array<int, MAX_TIMERS> timer_secs{};
    bool pos_valid = false;
    int  win_x = 0, win_y = 0, win_w = 340;
    Config() { timer_secs.fill(60); }
};

export bool config_write(const Config& c, std::ostream& f) {
    f << std::format("show_clk={}\nshow_sw={}\nshow_tmr={}\ntopmost={}\nnum_timers={}\n",
                     c.show_clk ? 1 : 0, c.show_sw ? 1 : 0,
                     c.show_tmr ? 1 : 0, c.topmost  ? 1 : 0,
                     c.num_timers);
    for (int i = 0; i < c.num_timers; ++i)
        f << std::format("timer{}={}\n", i, c.timer_secs[i]);
    if (c.pos_valid)
        f << std::format("win_x={}\nwin_y={}\nwin_w={}\n", c.win_x, c.win_y, c.win_w);
    return f.good();
}

export bool config_read(Config& c, std::istream& f) {
    bool has_x = false, has_y = false, has_w = false;
    for (std::string line; std::getline(f, line); ) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string_view key{line.data(), eq};
        std::string_view rest{line.data() + eq + 1, line.size() - eq - 1};
        int val;
        if (std::from_chars(rest.data(), rest.data() + rest.size(), val).ec != std::errc{})
            continue;
        if      (key == "show_clk")   c.show_clk   = val != 0;
        else if (key == "show_sw")    c.show_sw    = val != 0;
        else if (key == "show_tmr")   c.show_tmr   = val != 0;
        else if (key == "topmost")    c.topmost    = val != 0;
        else if (key == "num_timers") c.num_timers = std::clamp(val, 1, Config::MAX_TIMERS);
        else if (key == "win_x")    { c.win_x = val; has_x = true; }
        else if (key == "win_y")    { c.win_y = val; has_y = true; }
        else if (key == "win_w")    { c.win_w = std::max(Config::MIN_WINDOW_W, val); has_w = true; }
        else {
            for (int i = 0; i < Config::MAX_TIMERS; ++i)
                if (key == std::format("timer{}", i))
                    c.timer_secs[i] = std::clamp(val, Config::TIMER_MIN_SECS, Config::TIMER_MAX_SECS);
        }
    }
    c.pos_valid = has_x && has_y && has_w;
    return true;
}
