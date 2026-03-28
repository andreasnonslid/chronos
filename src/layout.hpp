#pragma once

constexpr int STANDARD_DPI = 96;

struct Layout {
    static constexpr int BASE_BAR_H  = 36;
    static constexpr int BASE_CLK_H  = 62;
    static constexpr int BASE_SW_H   = 116;
    static constexpr int BASE_TMR_H  = 114;
    static constexpr int BASE_BTN_H  = 28;
    static constexpr int BASE_W_PIN  = 44;
    static constexpr int BASE_W_CLK  = 48;
    static constexpr int BASE_W_SW   = 76;
    static constexpr int BASE_W_TMR  = 54;
    static constexpr int BASE_BAR_GAP = 6;

    int bar_h   = BASE_BAR_H;
    int clk_h   = BASE_CLK_H;
    int sw_h    = BASE_SW_H;
    int tmr_h   = BASE_TMR_H;
    int btn_h   = BASE_BTN_H;

    int w_pin   = BASE_W_PIN;
    int w_clk   = BASE_W_CLK;
    int w_sw    = BASE_W_SW;
    int w_tmr   = BASE_W_TMR;
    int bar_gap = BASE_BAR_GAP;

    int dpi = STANDARD_DPI;

    int bar_min_client_w() const { return w_pin + w_clk + w_sw + w_tmr + 3 * bar_gap + 2 * dpi_scale(8); }

    int dpi_scale(int value) const { return value * dpi / STANDARD_DPI; }

    void update_for_dpi(int new_dpi) {
        dpi = new_dpi;
        bar_h   = dpi_scale(BASE_BAR_H);
        clk_h   = dpi_scale(BASE_CLK_H);
        sw_h    = dpi_scale(BASE_SW_H);
        tmr_h   = dpi_scale(BASE_TMR_H);
        btn_h   = dpi_scale(BASE_BTN_H);
        w_pin   = dpi_scale(BASE_W_PIN);
        w_clk   = dpi_scale(BASE_W_CLK);
        w_sw    = dpi_scale(BASE_W_SW);
        w_tmr   = dpi_scale(BASE_W_TMR);
        bar_gap = dpi_scale(BASE_BAR_GAP);
    }
};

struct LayoutState {
    bool show_clk = true;
    bool show_sw = true;
    bool show_tmr = true;
    int timer_count = 1;
};

struct TimerMetrics {
    int abw;     // arrow button width
    int abh;     // arrow button height
    int up_off;  // y-offset for up arrows
    int td_off;  // y-offset for time display
    int dn_off;  // y-offset for down arrows
    int col_gap; // horizontal gap between h/m/s columns

    static TimerMetrics from(const Layout& layout) {
        int abh    = layout.dpi_scale(16);
        int up_off = layout.dpi_scale(4);
        int td_off = up_off + abh + layout.dpi_scale(2);
        int dn_off = td_off + layout.dpi_scale(40) + layout.dpi_scale(2);
        return {
            .abw    = layout.dpi_scale(34),
            .abh    = abh,
            .up_off = up_off,
            .td_off = td_off,
            .dn_off = dn_off,
            .col_gap = layout.dpi_scale(52),
        };
    }
};

inline int client_height_for(const Layout& layout, const LayoutState& state) {
    int h = layout.bar_h;
    if (state.show_clk) h += layout.clk_h;
    if (state.show_sw) h += layout.sw_h;
    if (state.show_tmr) h += state.timer_count * layout.tmr_h;
    return h;
}
