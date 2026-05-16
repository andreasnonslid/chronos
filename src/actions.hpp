#pragma once
#include <chrono>
#include <filesystem>
#include <optional>
#include "app.hpp"

enum Act {
    A_TOPMOST = 1,
    A_SHOW_CLK,
    A_SHOW_SW,
    A_SHOW_TMR,
    A_SW_START,
    A_SW_LAP,
    A_SW_RESET,
    A_SW_GET,
    A_SW_COPY,
    A_THEME,
    A_CLK_CYCLE,
    A_TMR_RST_ALL,
    A_SETTINGS,
    A_SHOW_ALARMS,
    A_ALARM_ADD,
    A_TMR_BASE    = 100,
    A_ALARM_DEL   = 500,
    A_ALARM_TOGGLE = 520,
};

constexpr int TMR_STRIDE = 12;
constexpr int A_TMR_HUP  = 0;
constexpr int A_TMR_HDN  = 1;
constexpr int A_TMR_MUP  = 2;
constexpr int A_TMR_MDN  = 3;
constexpr int A_TMR_SUP  = 4;
constexpr int A_TMR_SDN  = 5;
constexpr int A_TMR_START = 6;
constexpr int A_TMR_RST  = 7;
constexpr int A_TMR_ADD  = 8;
constexpr int A_TMR_DEL  = 9;
constexpr int A_TMR_POMO = 10;
constexpr int A_TMR_SKIP = 11;

struct HandleResult {
    bool save_config = false;
    bool resize = false;
    bool set_topmost = false;
    bool open_file = false;
    bool copy_laps = false;
    bool apply_theme = false;
    bool open_settings = false;
    bool open_alarm_dialog = false;
};

void reset_timer_slot(TimerSlot& ts, const App& app);
bool apply_timer_hms_adjust(TimerSlot& ts, int off);
int  tmr_act(int i, int off);

// Inverse of tmr_act: returns {idx, op_off} if `act` is a timer action, else
// std::nullopt. Centralises the (act - A_TMR_BASE) decomposition that used to
// be scattered across dispatch_action and wants_blink.
struct TmrDecoded { int idx; int off; };
inline std::optional<TmrDecoded> tmr_decode(int act) {
    if (act < A_TMR_BASE || act >= A_ALARM_DEL) return std::nullopt;
    int rel = act - A_TMR_BASE;
    return TmrDecoded{rel / TMR_STRIDE, rel % TMR_STRIDE};
}

bool wants_blink(int act);
HandleResult dispatch_timer_action(App& app, int idx, int off,
                                   std::chrono::steady_clock::time_point now);
HandleResult dispatch_action(App& app, int act,
                             std::chrono::steady_clock::time_point now,
                             const std::filesystem::path& config_dir);
