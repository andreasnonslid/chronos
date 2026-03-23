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

    int bar_min_client_w() const {
        return w_pin + w_clk + w_sw + w_tmr + 3 * bar_gap + 2 * 8;
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
