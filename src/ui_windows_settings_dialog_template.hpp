// Included by ui_windows.hpp inside namespace settings_dlg.

struct Buf {
    std::vector<WORD> data;
    void align4() { while (data.size() % 2) data.push_back(0); }
    void push_word(WORD w)   { data.push_back(w); }
    void push_dword(DWORD d) { data.push_back(LOWORD(d)); data.push_back(HIWORD(d)); }
    void push_wstr(const wchar_t* s) { while (*s) data.push_back((WORD)*s++); data.push_back(0); }
    void push_wstr_empty() { data.push_back(0); }
};

static void add_item(Buf& b, DWORD style, DWORD exStyle,
                     short x, short y, short cx, short cy,
                     WORD id, WORD cls_atom, const wchar_t* title) {
    b.align4();
    b.push_dword(WS_CHILD | WS_VISIBLE | style);
    b.push_dword(exStyle);
    b.push_word((WORD)x); b.push_word((WORD)y);
    b.push_word((WORD)cx); b.push_word((WORD)cy);
    b.push_word(id);
    b.push_word(0xFFFF); b.push_word(cls_atom);
    b.push_wstr(title);
    b.push_word(0);
}

constexpr WORD CTRL_COUNT = 30;

static std::vector<WORD> build_template() {
    Buf b;
    b.push_dword(WS_POPUP | WS_THICKFRAME | WS_VSCROLL);
    b.push_dword(0);
    b.push_word(CTRL_COUNT);
    b.push_word(0); b.push_word(0);
    b.push_word(DLG_W); b.push_word(DLG_H);
    b.push_wstr_empty();
    b.push_wstr_empty();
    b.push_wstr_empty();

    short lbl_x = 72, lbl_w = 50;
    short ed_x = 126, ed_w = 28;
    short min_x = 158, min_w = 18;
    short row_h = 10;
    short y0 = 40, sp = 18;

    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, y0, lbl_w, row_h, IDC_LBL_WORK, CLS_STATIC, L"Work");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, y0, ed_w, row_h, IDC_POMO_WORK, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, y0, min_w, row_h, IDC_MIN_WORK, CLS_STATIC, L"min");

    short r1 = y0 + sp;
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, r1, lbl_w, row_h, IDC_LBL_SHORT, CLS_STATIC, L"Short break");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, r1, ed_w, row_h, IDC_POMO_SHORT, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, r1, min_w, row_h, IDC_MIN_SHORT, CLS_STATIC, L"min");

    short r2 = y0 + 2 * sp;
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, r2, lbl_w, row_h, IDC_LBL_LONG, CLS_STATIC, L"Long break");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, r2, ed_w, row_h, IDC_POMO_LONG, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, r2, min_w, row_h, IDC_MIN_LONG, CLS_STATIC, L"min");

    short r3 = y0 + 3 * sp;
    add_item(b, SS_RIGHT | SS_CENTERIMAGE, 0,
             lbl_x, r3, lbl_w, row_h, IDC_LBL_CADENCE, CLS_STATIC, L"Long every");
    add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
             ed_x, r3, ed_w, row_h, IDC_POMO_CADENCE, CLS_EDIT, L"");
    add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
             min_x, r3, min_w + 20, row_h, IDC_MIN_CADENCE, CLS_STATIC, L"sessions");

    short pr_ed_x = 96, pr_ed_w = 28;
    short pr_min_x = 128, pr_min_w = 18;
    short pr_y0 = 40, pr_sp = 16;
    int pr_edit_ids[] = {IDC_PRESET_ED0, IDC_PRESET_ED1, IDC_PRESET_ED2,
                         IDC_PRESET_ED3, IDC_PRESET_ED4};
    int pr_min_ids[]  = {IDC_PRESET_MIN0, IDC_PRESET_MIN1, IDC_PRESET_MIN2,
                         IDC_PRESET_MIN3, IDC_PRESET_MIN4};
    for (int i = 0; i < 5; ++i) {
        short py = pr_y0 + (short)(i * pr_sp);
        add_item(b, ES_NUMBER | ES_CENTER | WS_TABSTOP, 0,
                 pr_ed_x, py, pr_ed_w, row_h, (WORD)pr_edit_ids[i], CLS_EDIT, L"");
        add_item(b, SS_LEFT | SS_CENTERIMAGE, 0,
                 pr_min_x, py, pr_min_w, row_h, (WORD)pr_min_ids[i], CLS_STATIC, L"min");
    }

    // Clock format dropdown — height includes dropdown list space for 5 items
    add_item(b, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0,
             68, 40, 212, 80, IDC_CLOCK_COMBO, CLS_COMBOBOX, L"");

    // Theme radio buttons (Appearance tab)
    add_item(b, WS_GROUP | WS_TABSTOP | BS_OWNERDRAW, 0,
             70, 42, 54, 13, IDC_THEME_AUTO, CLS_BUTTON, L"Auto");
    add_item(b, BS_OWNERDRAW, 0,
             70, 57, 54, 13, IDC_THEME_LIGHT, CLS_BUTTON, L"Light");
    add_item(b, BS_OWNERDRAW, 0,
             70, 72, 54, 13, IDC_THEME_DARK, CLS_BUTTON, L"Dark");

    // Sound checkbox (Appearance tab)
    add_item(b, WS_GROUP | WS_TABSTOP | BS_OWNERDRAW, 0,
             70, 97, 100, 13, IDC_SOUND, CLS_BUTTON, L"Sound");

    // Auto-start checkbox (Pomodoro tab)
    add_item(b, WS_GROUP | WS_TABSTOP | BS_OWNERDRAW, 0,
             70, 115, 100, 13, IDC_AUTO_START, CLS_BUTTON, L"Auto-start");

    short btn_w = 44, btn_h = 16, btn_gap = 8;
    short content_cx = DLG_W - SIDEBAR_W - 4;
    short total_bw = 2 * btn_w + btn_gap;
    short btn_x0 = SIDEBAR_W + 2 + (content_cx - total_bw) / 2;
    short btn_y = DLG_H - 24;
    add_item(b, BS_OWNERDRAW | WS_TABSTOP, 0,
             btn_x0, btn_y, btn_w, btn_h, IDC_SET_OK, CLS_BUTTON, L"Apply");
    add_item(b, BS_OWNERDRAW | WS_TABSTOP, 0,
             btn_x0 + btn_w + btn_gap, btn_y, btn_w, btn_h, IDC_SET_CANCEL, CLS_BUTTON, L"Cancel");

    return b.data;
}
