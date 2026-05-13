// Included by ui_windows.hpp inside namespace settings_dlg.

static Params* dialog_params(HWND dlg) {
    return reinterpret_cast<Params*>(GetWindowLongPtrW(dlg, DWLP_USER));
}

static void center_dialog_on_parent(HWND dlg) {
    HWND parent = GetParent(dlg);
    if (!parent) return;

    RECT pr{}, dr{};
    GetWindowRect(parent, &pr);
    GetWindowRect(dlg, &dr);
    int dw = dr.right - dr.left;
    int dh = dr.bottom - dr.top;
    int x = pr.left + (pr.right - pr.left - dw) / 2;
    int y = pr.top + (pr.bottom - pr.top - dh) / 2;
    SetWindowPos(dlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

static void tab_pomodoro_init(HWND dlg, Params& p) {
    SetDlgItemTextW(dlg, IDC_POMO_WORK,    std::format(L"{}", p.work_min).c_str());
    SetDlgItemTextW(dlg, IDC_POMO_SHORT,   std::format(L"{}", p.short_min).c_str());
    SetDlgItemTextW(dlg, IDC_POMO_LONG,    std::format(L"{}", p.long_min).c_str());
    SetDlgItemTextW(dlg, IDC_POMO_CADENCE, std::format(L"{}", p.cadence).c_str());
    SendDlgItemMessageW(dlg, IDC_POMO_WORK,    EM_SETLIMITTEXT, 4, 0);
    SendDlgItemMessageW(dlg, IDC_POMO_SHORT,   EM_SETLIMITTEXT, 4, 0);
    SendDlgItemMessageW(dlg, IDC_POMO_LONG,    EM_SETLIMITTEXT, 4, 0);
    SendDlgItemMessageW(dlg, IDC_POMO_CADENCE, EM_SETLIMITTEXT, 2, 0);
}

static void tab_timers_init(HWND dlg, Params& p) {
    int pr_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                    IDC_PRESET_ED3, IDC_PRESET_ED4};
    for (int i = 0; i < 5; ++i) {
        if (p.preset_min[i] > 0)
            SetDlgItemTextW(dlg, pr_ids[i], std::format(L"{}", p.preset_min[i]).c_str());
        SendDlgItemMessageW(dlg, pr_ids[i], EM_SETLIMITTEXT, 4, 0);
    }
}

static void tab_clock_init(HWND dlg, Params& p) {
    HWND combo = GetDlgItem(dlg, IDC_CLOCK_COMBO);
    if (!combo) return;

    const wchar_t* clock_names[CLOCK_VIEW_COUNT] = {
        L"24h + seconds", L"24h", L"12h + seconds", L"12h", L"Analog"
    };
    for (int i = 0; i < CLOCK_VIEW_COUNT; ++i)
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)clock_names[i]);
    SendMessageW(combo, CB_SETCURSEL, (WPARAM)(int)p.clock_view, 0);
}

static void set_active_tab(HWND dlg, Params& p, int tab) {
    set_pomo_visible(dlg, false);
    set_presets_visible(dlg, false);
    set_clock_combo_visible(dlg, false);

    p.active_tab = tab;

    set_pomo_visible(dlg, tab == TAB_POMODORO);
    set_presets_visible(dlg, tab == TAB_TIMERS);
    set_clock_combo_visible(dlg, tab == TAB_CLOCK);
    update_content_scroll(dlg, &p);
    InvalidateRect(dlg, nullptr, TRUE);
}

static void set_initial_tab_visibility(HWND dlg, Params& p) {
    set_pomo_visible(dlg, false);
    set_presets_visible(dlg, false);
    set_clock_combo_visible(dlg, false);
    update_content_scroll(dlg, &p);
}

static void set_child_fonts(HWND dlg, Params& p) {
    EnumChildWindows(dlg, [](HWND child, LPARAM param) -> BOOL {
        auto* params = reinterpret_cast<Params*>(param);
        SendMessageW(child, WM_SETFONT, (WPARAM)params->style.font, FALSE);
        return TRUE;
    }, (LPARAM)&p);
}


static int* analog_color_field(AnalogClockStyle& a, int i) {
    switch (i) {
    case 0: return &a.hour_color;
    case 1: return &a.minute_color;
    case 2: return &a.second_color;
    case 3: return &a.background_color;
    case 4: return &a.face_fill_color;
    case 5: return &a.face_outline_color;
    case 6: return &a.tick_color;
    case 7: return &a.hour_label_color;
    case 8: return &a.center_dot_color;
    default: return nullptr;
    }
}

static int* analog_value_field(AnalogClockStyle& a, int i) {
    switch (i) {
    case 0: return &a.hour_len_pct;
    case 1: return &a.minute_len_pct;
    case 2: return &a.second_len_pct;
    case 3: return &a.hour_thickness;
    case 4: return &a.minute_thickness;
    case 5: return &a.second_thickness;
    case 6: return &a.hour_tick_pct;
    case 7: return &a.minute_tick_pct;
    case 8: return &a.center_dot_size;
    case 9: return &a.hour_opacity_pct;
    case 10: return &a.minute_opacity_pct;
    case 11: return &a.second_opacity_pct;
    case 12: return &a.tick_opacity_pct;
    case 13: return &a.face_opacity_pct;
    case 14: return &a.radius_pct;
    default: return nullptr;
    }
}

static void analog_value_range(int i, int& min_v, int& max_v, int& step) {
    struct Range { int min_v, max_v, step; };
    static constexpr Range ranges[ANALOG_VALUE_COUNT] = {
        {0, 100, 5}, {0, 100, 5}, {0, 100, 5},
        {1, 6, 1}, {1, 4, 1}, {1, 3, 1},
        {5, 20, 1}, {2, 10, 1}, {0, 10, 1},
        {10, 100, 10}, {10, 100, 10}, {10, 100, 10},
        {10, 100, 10}, {0, 100, 10}, {50, 100, 5},
    };
    min_v = ranges[i].min_v;
    max_v = ranges[i].max_v;
    step = ranges[i].step;
}

static const wchar_t* analog_color_label(int i) {
    static constexpr const wchar_t* labels[ANALOG_COLOR_COUNT] = {
        L"Hour", L"Minute", L"Second", L"Background", L"Face fill",
        L"Outline", L"Ticks", L"Numbers", L"Center dot"
    };
    return labels[i];
}

static const wchar_t* analog_value_label(int i) {
    static constexpr const wchar_t* labels[ANALOG_VALUE_COUNT] = {
        L"Hour length", L"Minute length", L"Second length",
        L"Hour thickness", L"Minute thickness", L"Second thickness",
        L"Hour tick", L"Minute tick", L"Center dot",
        L"Hour opacity", L"Minute opacity", L"Second opacity",
        L"Tick opacity", L"Face opacity", L"Clock radius"
    };
    return labels[i];
}

static constexpr COLORREF ANALOG_PALETTE_COLORS[] = {
    RGB(220, 70, 70), RGB(236, 145, 42), RGB(240, 200, 60),
    RGB(86, 180, 100), RGB(65, 160, 220), RGB(160, 110, 230),
    RGB(235, 235, 235), RGB(35, 35, 35),
};
static constexpr int ANALOG_CUSTOM_PALETTE_COUNT =
    (int)(sizeof(ANALOG_PALETTE_COLORS) / sizeof(ANALOG_PALETTE_COLORS[0]));
static constexpr int ANALOG_PALETTE_COUNT = ANALOG_CUSTOM_PALETTE_COUNT + 1; // auto + custom colors

static COLORREF analog_palette_color(int palette_i) {
    if (palette_i == 0) return (COLORREF)-1;
    return ANALOG_PALETTE_COLORS[(palette_i - 1) % ANALOG_CUSTOM_PALETTE_COUNT];
}

static int analog_palette_index(int color) {
    if (color < 0) return 0;
    for (int i = 1; i < ANALOG_PALETTE_COUNT; ++i)
        if ((COLORREF)color == analog_palette_color(i)) return i;
    return 0;
}

static void reposition_buttons(HWND dlg) {
    HWND ok_btn = GetDlgItem(dlg, IDC_SET_OK);
    HWND cancel_btn = GetDlgItem(dlg, IDC_SET_CANCEL);
    if (!ok_btn || !cancel_btn) return;

    RECT rc{};
    GetClientRect(dlg, &rc);

    RECT ok_wr{}, cancel_wr{};
    GetWindowRect(ok_btn, &ok_wr);
    GetWindowRect(cancel_btn, &cancel_wr);
    int bw = ok_wr.right - ok_wr.left;
    int bh = ok_wr.bottom - ok_wr.top;
    int cw = cancel_wr.right - cancel_wr.left;

    RECT gaps{0, 0, 8, 4};
    MapDialogRect(dlg, &gaps);
    int gap = gaps.right;
    int bot_margin = gaps.bottom;

    RECT sb{0, 0, SIDEBAR_W, 0};
    MapDialogRect(dlg, &sb);
    int content_left = sb.right;
    int content_w = rc.right - content_left;
    int total_bw = bw + gap + cw;
    int btn_x0 = content_left + (content_w - total_bw) / 2;
    int btn_y = rc.bottom - bh - bot_margin;

    SetWindowPos(ok_btn,     nullptr, btn_x0,            btn_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(cancel_btn, nullptr, btn_x0 + bw + gap, btn_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

static INT_PTR on_resize(HWND dlg, WPARAM wp) {
    if (wp == SIZE_MINIMIZED) return FALSE;
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;
    reposition_buttons(dlg);
    update_content_scroll(dlg, p);
    InvalidateRect(dlg, nullptr, TRUE);
    return TRUE;
}

static INT_PTR on_get_min_max_info(HWND dlg, LPARAM lp) {
    auto* mm = reinterpret_cast<MINMAXINFO*>(lp);
    RECT r{0, 0, 200, 140};
    MapDialogRect(dlg, &r);
    int frame_x = GetSystemMetrics(SM_CXSIZEFRAME);
    int frame_y = GetSystemMetrics(SM_CYSIZEFRAME);
    mm->ptMinTrackSize = {r.right + 2 * frame_x, r.bottom + 2 * frame_y};
    return TRUE;
}

static INT_PTR on_init(HWND dlg, LPARAM lp) {
    auto* p = reinterpret_cast<Params*>(lp);
    SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)p);
    p->style.apply_dark_mode(dlg);
    center_dialog_on_parent(dlg);

    p->rects = compute_rects(dlg);
    p->clock_combo_rc = map_dlu(dlg, 68, 40, 212, 80);

    tab_pomodoro_init(dlg, *p);
    tab_timers_init(dlg, *p);
    tab_clock_init(dlg, *p);
    set_child_fonts(dlg, *p);
    set_initial_tab_visibility(dlg, *p);
    reposition_buttons(dlg);
    return FALSE;
}

static INT_PTR on_erase_background(HWND dlg, HDC hdc) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    RECT rc{};
    GetClientRect(dlg, &rc);
    auto& s = p->style;

    s.fill_background(hdc, rc);

    RECT div_y = {0, 0, 0, TITLE_H + 2};
    MapDialogRect(dlg, &div_y);
    RECT sb = {0, 0, SIDEBAR_W, DLG_H};
    MapDialogRect(dlg, &sb);
    sb.top = div_y.bottom;
    sb.bottom = rc.bottom;
    HBRUSH bar_br = CreateSolidBrush(s.theme->bar);
    FillRect(hdc, &sb, bar_br);
    DeleteObject(bar_br);

    RECT title_rc = {0, 0, rc.right, div_y.bottom};
    s.draw_label(hdc, title_rc, L"Settings", DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HPEN div_pen = CreatePen(PS_SOLID, 1, s.theme->divider);
    auto* old_pen = (HPEN)SelectObject(hdc, div_pen);

    MoveToEx(hdc, 0, div_y.bottom, nullptr);
    LineTo(hdc, rc.right, div_y.bottom);

    MoveToEx(hdc, sb.right, div_y.bottom, nullptr);
    LineTo(hdc, sb.right, rc.bottom);

    int btn_area_top = content_area_bottom(dlg);
    MoveToEx(hdc, sb.right, btn_area_top, nullptr);
    LineTo(hdc, rc.right, btn_area_top);

    SelectObject(hdc, old_pen);
    DeleteObject(div_pen);

    const wchar_t* tab_names[] = {L"Appearance", L"Clock", L"Pomodoro", L"Timers"};
    for (int i = 0; i < TAB_COUNT; ++i)
        paint_option_btn(hdc, p->rects.sidebar[i], tab_names[i], i == p->active_tab, s);

    int save_dc = SaveDC(hdc);
    IntersectClipRect(hdc, sb.right + 1, div_y.bottom + 1, rc.right, btn_area_top);

    int sdy = -p->scroll_y;

    if (p->active_tab == TAB_APPEARANCE) {
        RECT lbl = map_dlu(dlg, 70, 28, 80, 12);
        lbl.top += sdy; lbl.bottom += sdy;
        s.draw_label(hdc, lbl, L"Theme");

        const wchar_t* names[] = {L"Auto", L"Light", L"Dark"};
        for (int i = 0; i < 3; ++i) {
            RECT r = p->rects.theme[i];
            r.top += sdy; r.bottom += sdy;
            paint_option_btn(hdc, r, names[i], (int)p->theme_mode == i, s);
        }

        RECT slbl = map_dlu(dlg, 70, 88, 80, 8);
        slbl.top += sdy; slbl.bottom += sdy;
        s.draw_label(hdc, slbl, L"Notifications");

        RECT sndr = p->rects.sound;
        sndr.top += sdy; sndr.bottom += sdy;
        paint_option_btn(hdc, sndr,
                         p->sound_on_expiry ? L"Sound: on" : L"Sound: off",
                         p->sound_on_expiry, s);

    } else if (p->active_tab == TAB_POMODORO) {
        RECT aslbl = map_dlu(dlg, 70, 105, 80, 8);
        aslbl.top += sdy; aslbl.bottom += sdy;
        s.draw_label(hdc, aslbl, L"Auto-start");

        RECT asr = p->rects.auto_start;
        asr.top += sdy; asr.bottom += sdy;
        paint_option_btn(hdc, asr,
                         p->auto_start ? L"On" : L"Off",
                         p->auto_start, s);

    } else if (p->active_tab == TAB_CLOCK) {
        RECT lbl = map_dlu(dlg, 70, 28, 80, 12);
        lbl.top += sdy; lbl.bottom += sdy;
        s.draw_label(hdc, lbl, L"Format");

        if (p->clock_view == ClockView::Analog) {
            RECT preview = p->rects.analog_preview;

            // Draw scrollable settings clipped to exclude the sticky preview region
            {
                int sdc2 = SaveDC(hdc);
                ExcludeClipRect(hdc, preview.left, preview.top, preview.right, preview.bottom);

                RECT min_r = p->rects.analog_min_ticks;
                min_r.top += sdy; min_r.bottom += sdy;
                paint_option_btn(hdc, min_r,
                                 p->analog_style.show_minute_ticks ? L"Min ticks: on" : L"Min ticks: off",
                                 p->analog_style.show_minute_ticks, s);

                RECT hrlbl = map_dlu(dlg, 158, 82, 80, 8);
                hrlbl.top += sdy; hrlbl.bottom += sdy;
                s.draw_label(hdc, hrlbl, L"Hour labels");

                const wchar_t* lbl_names[] = {L"None", L"Sparse", L"Full"};
                for (int i = 0; i < 3; ++i) {
                    RECT ar = p->rects.analog_labels[i];
                    ar.top += sdy; ar.bottom += sdy;
                    paint_option_btn(hdc, ar, lbl_names[i],
                                     (int)p->analog_style.hour_labels == i, s);
                }

                RECT color_lbl = map_dlu(dlg, 158, 112, 80, 8);
                color_lbl.top += sdy; color_lbl.bottom += sdy;
                s.draw_label(hdc, color_lbl, L"Colors");
                for (int i = 0; i < ANALOG_COLOR_COUNT; ++i) {
                    RECT r = p->rects.analog_colors[i];
                    r.top += sdy; r.bottom += sdy;
                    int* field = analog_color_field(p->analog_style, i);
                    bool custom = field && *field >= 0;
                    paint_option_btn(hdc, r, analog_color_label(i), custom, s);
                    RECT sw = {r.right - s.scale(14), r.top + s.scale(2), r.right - s.scale(3), r.bottom - s.scale(2)};
                    COLORREF sw_color = custom ? (COLORREF)*field : s.theme->dim;
                    GdiObj br{CreateSolidBrush(sw_color)};
                    FillRect(hdc, &sw, (HBRUSH)br.h);
                }

                RECT value_lbl = map_dlu(dlg, 68, 248, 200, 8);
                value_lbl.top += sdy; value_lbl.bottom += sdy;
                s.draw_label(hdc, value_lbl, L"Size and opacity");
                for (int i = 0; i < ANALOG_VALUE_COUNT; ++i) {
                    RECT r = p->rects.analog_values[i];
                    r.top += sdy; r.bottom += sdy;
                    int* field = analog_value_field(p->analog_style, i);
                    wchar_t buf[96];
                    wsprintfW(buf, L"%s: %d", analog_value_label(i), field ? *field : 0);
                    paint_option_btn(hdc, r, buf, false, s);
                }

                RestoreDC(hdc, sdc2);
            }

            // Fixed preview drawn on top — no sdy offset
            RECT prev_lbl = map_dlu(dlg, 68, 48, 80, 8);
            s.draw_label(hdc, prev_lbl, L"Live preview");
            draw_analog_clock(hdc, preview, p->analog_style, *s.theme, s.dpi, 10, 10, 30);
        }

    } else if (p->active_tab == TAB_TIMERS) {
        RECT lbl = map_dlu(dlg, 70, 28, 100, 12);
        lbl.top += sdy; lbl.bottom += sdy;
        s.draw_label(hdc, lbl, L"Custom presets");

        for (int i = 0; i < 5; ++i) {
            RECT num = map_dlu(dlg, 76, (short)(40 + i * 16), 16, 12);
            num.top += sdy; num.bottom += sdy;
            auto txt = std::format(L"{}.", i + 1);
            s.draw_label(hdc, num, txt.c_str());
        }
    }

    RestoreDC(hdc, save_dc);
    return TRUE;
}

static INT_PTR on_ctl_color(HWND dlg, UINT msg, HDC hdc) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    switch (msg) {
    case WM_CTLCOLORSTATIC: {
        SetTextColor(hdc, p->style.theme->text);
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH s_bg = nullptr;
        if (s_bg) DeleteObject(s_bg);
        s_bg = CreateSolidBrush(p->style.theme->bg);
        return (INT_PTR)s_bg;
    }
    case WM_CTLCOLOREDIT: {
        SetTextColor(hdc, p->style.theme->text);
        SetBkColor(hdc, p->style.theme->bar);
        static HBRUSH s_edit_bg = nullptr;
        if (s_edit_bg) DeleteObject(s_edit_bg);
        s_edit_bg = CreateSolidBrush(p->style.theme->bar);
        return (INT_PTR)s_edit_bg;
    }
    case WM_CTLCOLORBTN: {
        static HBRUSH s_btn_bg = nullptr;
        if (s_btn_bg) DeleteObject(s_btn_bg);
        s_btn_bg = CreateSolidBrush(p->style.theme->bg);
        return (INT_PTR)s_btn_bg;
    }
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORCOMBOBOX: {
        SetTextColor(hdc, p->style.theme->text);
        SetBkColor(hdc, p->style.theme->bar);
        static HBRUSH s_combo_bg = nullptr;
        if (s_combo_bg) DeleteObject(s_combo_bg);
        s_combo_bg = CreateSolidBrush(p->style.theme->bar);
        return (INT_PTR)s_combo_bg;
    }
    }
    return FALSE;
}

static INT_PTR on_nc_hit_test(HWND dlg, LPARAM lp) {
    RECT rc{};
    GetClientRect(dlg, &rc);
    RECT title_h{0, 0, 0, TITLE_H + 2};
    MapDialogRect(dlg, &title_h);
    POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    ScreenToClient(dlg, &pt);
    if (pt.x >= 0 && pt.x < rc.right && pt.y >= 0 && pt.y < title_h.bottom) {
        SetWindowLongPtrW(dlg, DWLP_MSGRESULT, HTCAPTION);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR on_draw_item(HWND dlg, LPARAM lp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    auto* di = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
    if (di->CtlType != ODT_BUTTON) return FALSE;

    const wchar_t* label = (di->CtlID == IDC_SET_OK) ? L"Apply" : L"Cancel";
    bool focused = (di->itemState & ODS_FOCUS) != 0;
    p->style.draw_button(di->hDC, di->rcItem, label, focused);
    return TRUE;
}

static INT_PTR on_left_button_down(HWND dlg, LPARAM lp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

    for (int i = 0; i < TAB_COUNT; ++i) {
        if (PtInRect(&p->rects.sidebar[i], pt) && i != p->active_tab) {
            set_active_tab(dlg, *p, i);
            return TRUE;
        }
    }

    POINT spt = {pt.x, pt.y + p->scroll_y};

    if (p->active_tab == TAB_APPEARANCE) {
        for (int i = 0; i < 3; ++i) {
            if (PtInRect(&p->rects.theme[i], spt)) {
                p->theme_mode = (ThemeMode)i;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }
        if (PtInRect(&p->rects.sound, spt)) {
            p->sound_on_expiry = !p->sound_on_expiry;
            InvalidateRect(dlg, nullptr, TRUE);
            return TRUE;
        }
    }

    if (p->active_tab == TAB_CLOCK && p->clock_view == ClockView::Analog) {
        if (PtInRect(&p->rects.analog_min_ticks, spt)) {
            p->analog_style.show_minute_ticks = !p->analog_style.show_minute_ticks;
            InvalidateRect(dlg, nullptr, TRUE);
            return TRUE;
        }
        for (int i = 0; i < 3; ++i) {
            if (PtInRect(&p->rects.analog_labels[i], spt)) {
                p->analog_style.hour_labels = (HourLabels)i;
                InvalidateRect(dlg, nullptr, TRUE);
                return TRUE;
            }
        }
        for (int i = 0; i < ANALOG_COLOR_COUNT; ++i) {
            if (PtInRect(&p->rects.analog_colors[i], spt)) {
                int* field = analog_color_field(p->analog_style, i);
                if (field) {
                    int next = (analog_palette_index(*field) + 1) % ANALOG_PALETTE_COUNT;
                    *field = (int)analog_palette_color(next);
                    InvalidateRect(dlg, nullptr, TRUE);
                    return TRUE;
                }
            }
        }
        for (int i = 0; i < ANALOG_VALUE_COUNT; ++i) {
            if (PtInRect(&p->rects.analog_values[i], spt)) {
                int* field = analog_value_field(p->analog_style, i);
                if (field) {
                    int min_v, max_v, step;
                    analog_value_range(i, min_v, max_v, step);
                    *field += step;
                    if (*field > max_v) *field = min_v;
                    InvalidateRect(dlg, nullptr, TRUE);
                    return TRUE;
                }
            }
        }
    }

    if (p->active_tab == TAB_POMODORO && PtInRect(&p->rects.auto_start, spt)) {
        p->auto_start = !p->auto_start;
        InvalidateRect(dlg, nullptr, TRUE);
        return TRUE;
    }

    return FALSE;
}

static INT_PTR on_scroll(HWND dlg, WPARAM wp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0};
    GetScrollInfo(dlg, SB_VERT, &si);
    int new_pos = si.nPos;
    switch (LOWORD(wp)) {
    case SB_LINEUP:     new_pos -= 10; break;
    case SB_LINEDOWN:   new_pos += 10; break;
    case SB_PAGEUP:     new_pos -= (int)si.nPage; break;
    case SB_PAGEDOWN:   new_pos += (int)si.nPage; break;
    case SB_THUMBTRACK: new_pos = si.nTrackPos; break;
    case SB_TOP:        new_pos = si.nMin; break;
    case SB_BOTTOM:     new_pos = si.nMax; break;
    }
    apply_scroll(dlg, p, new_pos);
    return TRUE;
}

static INT_PTR on_mouse_wheel(HWND dlg, WPARAM wp) {
    auto* p = dialog_params(dlg);
    if (!p) return FALSE;

    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0};
    GetScrollInfo(dlg, SB_VERT, &si);
    if ((int)(si.nMax - si.nPage) <= si.nMin) return FALSE;

    int delta = GET_WHEEL_DELTA_WPARAM(wp);
    apply_scroll(dlg, p, si.nPos + (delta > 0 ? -30 : 30));
    return TRUE;
}

static bool tab_pomodoro_commit(HWND dlg, Params& p) {
    int w = 0, s = 0, l = 0, cad = 0;
    if (!read_field(dlg, IDC_POMO_WORK, w) ||
        !read_field(dlg, IDC_POMO_SHORT, s) ||
        !read_field(dlg, IDC_POMO_LONG, l)) {
        set_active_tab(dlg, p, TAB_POMODORO);
        MessageBeep(MB_ICONASTERISK);
        int dummy = 0;
        if (!read_field(dlg, IDC_POMO_WORK, dummy))
            SetFocus(GetDlgItem(dlg, IDC_POMO_WORK));
        else if (!read_field(dlg, IDC_POMO_SHORT, dummy))
            SetFocus(GetDlgItem(dlg, IDC_POMO_SHORT));
        else
            SetFocus(GetDlgItem(dlg, IDC_POMO_LONG));
        return false;
    }

    if (!read_field(dlg, IDC_POMO_CADENCE, cad) ||
        cad < POMODORO_MIN_CADENCE || cad > POMODORO_MAX_CADENCE) {
        set_active_tab(dlg, p, TAB_POMODORO);
        MessageBeep(MB_ICONASTERISK);
        SetFocus(GetDlgItem(dlg, IDC_POMO_CADENCE));
        return false;
    }

    p.work_min = w;
    p.short_min = s;
    p.long_min = l;
    p.cadence = cad;
    return true;
}

static bool tab_timers_commit(HWND dlg, Params& p) {
    int pr_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                    IDC_PRESET_ED3, IDC_PRESET_ED4};
    for (int i = 0; i < 5; ++i) {
        wchar_t buf[8] = {};
        GetDlgItemTextW(dlg, pr_ids[i], buf, 7);
        if (buf[0] == L'\0') {
            p.preset_min[i] = 0;
            continue;
        }
        wchar_t* end = nullptr;
        long v = std::wcstol(buf, &end, 10);
        p.preset_min[i] = (v >= 1 && v <= 1440) ? (int)v : 0;
    }
    return true;
}

static bool commit_all_tabs(HWND dlg, Params& p) {
    return tab_pomodoro_commit(dlg, p) && tab_timers_commit(dlg, p);
}

static INT_PTR on_command(HWND dlg, WPARAM wp) {
    switch (LOWORD(wp)) {
    case IDC_CLOCK_COMBO:
        if (HIWORD(wp) == CBN_SELCHANGE) {
            auto* p = dialog_params(dlg);
            if (!p) return FALSE;
            HWND combo = GetDlgItem(dlg, IDC_CLOCK_COMBO);
            int sel = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < CLOCK_VIEW_COUNT) {
                p->clock_view = (ClockView)sel;
                update_content_scroll(dlg, p);
                InvalidateRect(dlg, nullptr, TRUE);
            }
        }
        return TRUE;
    case IDC_SET_OK: {
        auto* p = dialog_params(dlg);
        if (!p) return FALSE;
        if (!commit_all_tabs(dlg, *p)) return TRUE;
        EndDialog(dlg, IDOK);
        return TRUE;
    }
    case IDC_SET_CANCEL:
        EndDialog(dlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR on_key_down(HWND dlg, WPARAM wp) {
    if (wp == VK_ESCAPE) {
        EndDialog(dlg, IDCANCEL);
        return TRUE;
    }
    if (wp == VK_RETURN) {
        SendMessageW(dlg, WM_COMMAND, IDC_SET_OK, 0);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG:       return on_init(dlg, lp);
    case WM_SIZE:             return on_resize(dlg, wp);
    case WM_GETMINMAXINFO:    return on_get_min_max_info(dlg, lp);
    case WM_ERASEBKGND:       return on_erase_background(dlg, (HDC)wp);
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORCOMBOBOX: return on_ctl_color(dlg, msg, (HDC)wp);
    case WM_NCHITTEST:        return on_nc_hit_test(dlg, lp);
    case WM_DRAWITEM:         return on_draw_item(dlg, lp);
    case WM_LBUTTONDOWN:      return on_left_button_down(dlg, lp);
    case WM_VSCROLL:          return on_scroll(dlg, wp);
    case WM_MOUSEWHEEL:       return on_mouse_wheel(dlg, wp);
    case WM_COMMAND:          return on_command(dlg, wp);
    case WM_KEYDOWN:          return on_key_down(dlg, wp);
    }
    return FALSE;
}
