export module layout;

export struct Layout {
    int bar_h  =  36;
    int clk_h  =  62;
    int sw_h   = 100;
    int tmr_h  = 118;
    int btn_h  =  28;

    int w_pin  = 44;
    int w_clk  = 48;
    int w_sw   = 76;
    int w_tmr  = 54;
    int bar_gap = 6;

    int dpi    = 96;

    int bar_min_client_w() const {
        return w_pin + w_clk + w_sw + w_tmr + 3 * bar_gap + 2 * dpi_scale(8);
    }

    int dpi_scale(int value) const {
        return value * dpi / 96;
    }

    void update_for_dpi(int new_dpi) {
        dpi     = new_dpi;
        bar_h   = dpi_scale(36);
        clk_h   = dpi_scale(62);
        sw_h    = dpi_scale(100);
        tmr_h   = dpi_scale(118);
        btn_h   = dpi_scale(28);
        w_pin   = dpi_scale(44);
        w_clk   = dpi_scale(48);
        w_sw    = dpi_scale(76);
        w_tmr   = dpi_scale(54);
        bar_gap = dpi_scale(6);
    }
};

export struct LayoutState {
    bool show_clk = true;
    bool show_sw = true;
    bool show_tmr = true;
    int timer_count = 1;
};

export int client_height_for(const Layout& layout, const LayoutState& state) {
    int h = layout.bar_h;
    if (state.show_clk) h += layout.clk_h;
    if (state.show_sw)  h += layout.sw_h;
    if (state.show_tmr) h += state.timer_count * layout.tmr_h;
    return h;
}
