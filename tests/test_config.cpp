#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>
#include "../src/config_serial.hpp"

TEST_CASE("Config defaults", "[config]") {
    Config c;
    REQUIRE(c.show_clk);
    REQUIRE(c.show_sw);
    REQUIRE(c.show_tmr);
    REQUIRE_FALSE(c.topmost);
    REQUIRE(c.num_timers == 1);
    REQUIRE(c.timer_secs[0] == 60);
    REQUIRE_FALSE(c.pos_valid);
    REQUIRE(c.win_w == 340);
}

TEST_CASE("Config round-trip write then read", "[config]") {
    Config orig;
    orig.show_clk = false;
    orig.show_sw = true;
    orig.show_tmr = false;
    orig.topmost = true;
    orig.num_timers = 2;
    orig.timer_secs[0] = 120;
    orig.timer_secs[1] = 300;
    orig.pos_valid = true;
    orig.win_x = 100;
    orig.win_y = 200;
    orig.win_w = 400;

    std::ostringstream os;
    REQUIRE(config_write(orig, os));

    Config read_back;
    std::istringstream is(os.str());
    REQUIRE(config_read(read_back, is));

    REQUIRE(read_back.show_clk == orig.show_clk);
    REQUIRE(read_back.show_sw == orig.show_sw);
    REQUIRE(read_back.show_tmr == orig.show_tmr);
    REQUIRE(read_back.topmost == orig.topmost);
    REQUIRE(read_back.num_timers == orig.num_timers);
    REQUIRE(read_back.timer_secs[0] == orig.timer_secs[0]);
    REQUIRE(read_back.timer_secs[1] == orig.timer_secs[1]);
    REQUIRE(read_back.pos_valid);
    REQUIRE(read_back.win_x == orig.win_x);
    REQUIRE(read_back.win_y == orig.win_y);
    REQUIRE(read_back.win_w == orig.win_w);
}

TEST_CASE("Config timer values clamped to 0-86400", "[config]") {
    std::istringstream is("timer0=-5\ntimer1=100000\n");
    Config c;
    c.num_timers = 2;
    config_read(c, is);
    REQUIRE(c.timer_secs[0] == 0);
    REQUIRE(c.timer_secs[1] == 86400);
}

TEST_CASE("Config partial file uses existing defaults", "[config]") {
    std::istringstream is("show_clk=0\n");
    Config c;
    config_read(c, is);
    REQUIRE_FALSE(c.show_clk);
    REQUIRE(c.show_sw);
    REQUIRE(c.show_tmr);
    REQUIRE(c.num_timers == 1);
}

TEST_CASE("Config corrupt non-numeric values are skipped", "[config]") {
    std::istringstream is("show_clk=abc\nshow_sw=0\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.show_clk);      // unchanged from default
    REQUIRE_FALSE(c.show_sw); // parsed successfully
}

TEST_CASE("Config win_w clamped to minimum 260", "[config]") {
    std::istringstream is("win_x=0\nwin_y=0\nwin_w=50\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.win_w == 260);
    REQUIRE(c.pos_valid);
}

TEST_CASE("Config pos_valid requires all three win fields", "[config]") {
    {
        std::istringstream is("win_x=0\nwin_y=0\n");
        Config c;
        config_read(c, is);
        REQUIRE_FALSE(c.pos_valid);
    }
    {
        std::istringstream is("win_x=0\nwin_w=300\n");
        Config c;
        config_read(c, is);
        REQUIRE_FALSE(c.pos_valid);
    }
}

TEST_CASE("Config num_timers clamped to 1..MAX_TIMERS", "[config]") {
    {
        std::istringstream is("num_timers=0\n");
        Config c;
        config_read(c, is);
        REQUIRE(c.num_timers == 1);
    }
    {
        std::istringstream is("num_timers=99\n");
        Config c;
        config_read(c, is);
        REQUIRE(c.num_timers == Config::MAX_TIMERS);
    }
}

TEST_CASE("Config timer labels round-trip", "[config]") {
    Config orig;
    orig.num_timers = 2;
    orig.timer_labels[0] = "Tea";
    orig.timer_labels[1] = "Break";

    std::ostringstream os;
    config_write(orig, os);

    Config read_back;
    std::istringstream is(os.str());
    config_read(read_back, is);

    REQUIRE(read_back.timer_labels[0] == "Tea");
    REQUIRE(read_back.timer_labels[1] == "Break");
    REQUIRE(read_back.timer_labels[2].empty());
}

TEST_CASE("Config timer labels truncated to 20 chars", "[config]") {
    std::istringstream is("timer0_label=abcdefghijklmnopqrstuvwxyz\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.timer_labels[0].size() == 20);
}

TEST_CASE("Config empty stream", "[config]") {
    std::istringstream is("");
    Config c;
    REQUIRE(config_read(c, is));
    // All defaults preserved
    REQUIRE(c.show_clk);
    REQUIRE(c.num_timers == 1);
}

TEST_CASE("Config sw_lap_file round-trip", "[config]") {
    Config orig;
    orig.sw_running = true;
    orig.sw_elapsed_ms = 5000;
    orig.sw_lap_file = "/tmp/stopwatch-20260328-120000-000.txt";

    std::ostringstream os;
    config_write(orig, os);

    Config read_back;
    std::istringstream is(os.str());
    config_read(read_back, is);

    REQUIRE(read_back.sw_lap_file == orig.sw_lap_file);
}

TEST_CASE("Config sw_lap_file not written when sw state absent", "[config]") {
    Config orig;
    // sw_elapsed_ms == 0, sw_running == false — sw block should be skipped entirely
    orig.sw_lap_file = "/tmp/some-file.txt";

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("sw_lap_file") == std::string::npos);
}

TEST_CASE("Config sw_lap_file empty by default", "[config]") {
    Config c;
    REQUIRE(c.sw_lap_file.empty());
}

TEST_CASE("Config theme round-trip", "[config]") {
    for (auto mode : {ThemeMode::Auto, ThemeMode::Dark, ThemeMode::Light}) {
        Config orig;
        orig.theme_mode = mode;
        std::ostringstream os;
        config_write(orig, os);
        Config back;
        std::istringstream is(os.str());
        config_read(back, is);
        REQUIRE(back.theme_mode == mode);
    }
}

TEST_CASE("Config clock_view round-trip", "[config]") {
    for (auto view : {ClockView::H24_HMS, ClockView::H24_HM, ClockView::H12_HMS, ClockView::H12_HM, ClockView::Analog}) {
        Config orig;
        orig.clock_view = view;
        std::ostringstream os;
        config_write(orig, os);
        Config back;
        std::istringstream is(os.str());
        config_read(back, is);
        REQUIRE(back.clock_view == view);
    }
}

TEST_CASE("Config clock_view invalid value clamped", "[config]") {
    std::istringstream is("clock_view=99\n");
    Config c;
    config_read(c, is);
    REQUIRE((int)c.clock_view >= 0);
    REQUIRE((int)c.clock_view < CLOCK_VIEW_COUNT);
}

TEST_CASE("Config stopwatch runtime state round-trip", "[config]") {
    Config orig;
    orig.sw_running = true;
    orig.sw_elapsed_ms = 12345;
    orig.sw_start_epoch_ms = 1700000000000LL;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.sw_running);
    REQUIRE(back.sw_elapsed_ms == 12345);
    REQUIRE(back.sw_start_epoch_ms == 1700000000000LL);
}

TEST_CASE("Config stopwatch paused state round-trip", "[config]") {
    Config orig;
    orig.sw_running = false;
    orig.sw_elapsed_ms = 5000;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE_FALSE(back.sw_running);
    REQUIRE(back.sw_elapsed_ms == 5000);
    REQUIRE(os.str().find("sw_start_epoch_ms") == std::string::npos);
}

TEST_CASE("Config timer runtime state round-trip", "[config]") {
    Config orig;
    orig.num_timers = 2;
    orig.timer_secs[0] = 300;
    orig.timer_running[0] = true;
    orig.timer_elapsed_ms[0] = 5000;
    orig.timer_start_epoch_ms[0] = 1700000000000LL;
    orig.timer_notified[1] = true;
    orig.timer_elapsed_ms[1] = 60000;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.timer_running[0]);
    REQUIRE(back.timer_elapsed_ms[0] == 5000);
    REQUIRE(back.timer_start_epoch_ms[0] == 1700000000000LL);
    REQUIRE(back.timer_notified[1]);
    REQUIRE(back.timer_elapsed_ms[1] == 60000);
}

TEST_CASE("Config timer start_epoch_ms only written when running", "[config]") {
    Config orig;
    orig.num_timers = 1;
    orig.timer_running[0] = false;
    orig.timer_elapsed_ms[0] = 3000;
    orig.timer_start_epoch_ms[0] = 1700000000000LL;

    std::ostringstream os;
    config_write(orig, os);

    REQUIRE(os.str().find("timer0_start_epoch_ms") == std::string::npos);
    REQUIRE(os.str().find("timer0_running") == std::string::npos);
}

TEST_CASE("Config negative sw_elapsed_ms clamped to 0", "[config]") {
    std::istringstream is("sw_elapsed_ms=-1000\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.sw_elapsed_ms == 0);
}

TEST_CASE("Config negative sw_start_epoch_ms clamped to 0", "[config]") {
    std::istringstream is("sw_start_epoch_ms=-500\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.sw_start_epoch_ms == 0);
}

TEST_CASE("Config negative timer_elapsed_ms clamped to 0", "[config]") {
    std::istringstream is("timer0_elapsed_ms=-2500\ntimer1_elapsed_ms=-1\n");
    Config c;
    c.num_timers = 2;
    config_read(c, is);
    REQUIRE(c.timer_elapsed_ms[0] == 0);
    REQUIRE(c.timer_elapsed_ms[1] == 0);
}

TEST_CASE("Config negative timer_start_epoch_ms clamped to 0", "[config]") {
    std::istringstream is("timer0_start_epoch_ms=-999\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.timer_start_epoch_ms[0] == 0);
}

TEST_CASE("Config integer overflow timer value clamped to max", "[config]") {
    std::istringstream is("timer0=99999999999999999\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.timer_secs[0] == Config::TIMER_MAX_SECS);
}

TEST_CASE("Config int-wrapping timer value clamped to max", "[config]") {
    // 2^32 + 60 = 4294967356 would wrap to 60 with naive (int) cast
    std::istringstream is("timer0=4294967356\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.timer_secs[0] == Config::TIMER_MAX_SECS);
}

TEST_CASE("Config integer overflow num_timers clamped", "[config]") {
    std::istringstream is("num_timers=99999999999999999\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.num_timers >= 1);
    REQUIRE(c.num_timers <= Config::MAX_TIMERS);
}

TEST_CASE("Config integer overflow pomodoro values clamped", "[config]") {
    std::istringstream is("pomodoro_work=99999999999999999\npomodoro_short=99999999999999999\npomodoro_long=99999999999999999\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.pomodoro_work_secs <= Config::TIMER_MAX_SECS);
    REQUIRE(c.pomodoro_short_secs <= Config::TIMER_MAX_SECS);
    REQUIRE(c.pomodoro_long_secs <= Config::TIMER_MAX_SECS);
}

TEST_CASE("Config epoch zero with running stopwatch", "[config]") {
    std::istringstream is("sw_running=1\nsw_start_epoch_ms=0\nsw_elapsed_ms=5000\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.sw_running);
    REQUIRE(c.sw_start_epoch_ms == 0);
    REQUIRE(c.sw_elapsed_ms == 5000);
}

TEST_CASE("Config duplicate keys last value wins", "[config]") {
    std::istringstream is("show_clk=0\nshow_clk=1\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.show_clk);
}

TEST_CASE("Config empty label value produces empty string", "[config]") {
    std::istringstream is("timer0_label=\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.timer_labels[0].empty());
}

TEST_CASE("Config sound_on_expiry defaults to true", "[config]") {
    Config c;
    REQUIRE(c.sound_on_expiry);
}

TEST_CASE("Config sound_on_expiry round-trip", "[config]") {
    for (bool val : {true, false}) {
        Config orig;
        orig.sound_on_expiry = val;
        std::ostringstream os;
        config_write(orig, os);
        Config back;
        std::istringstream is(os.str());
        config_read(back, is);
        REQUIRE(back.sound_on_expiry == val);
    }
}

TEST_CASE("Config whitespace around equals is ignored", "[config]") {
    std::istringstream is("timer0 = 60\nshow_clk = 0\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.timer_secs[0] == 60);  // default unchanged, line skipped
    REQUIRE(c.show_clk);             // default unchanged, line skipped
}

TEST_CASE("Config custom presets round-trip", "[config]") {
    Config orig;
    orig.num_custom_presets = 3;
    orig.custom_preset_secs[0] = 420;
    orig.custom_preset_secs[1] = 900;
    orig.custom_preset_secs[2] = 2700;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.num_custom_presets == 3);
    REQUIRE(back.custom_preset_secs[0] == 420);
    REQUIRE(back.custom_preset_secs[1] == 900);
    REQUIRE(back.custom_preset_secs[2] == 2700);
}

TEST_CASE("Config custom presets not written when empty", "[config]") {
    Config orig;
    std::ostringstream os;
    config_write(orig, os);
    REQUIRE(os.str().find("custom_preset") == std::string::npos);
    REQUIRE(os.str().find("num_custom_presets") == std::string::npos);
}

TEST_CASE("Config custom presets count clamped", "[config]") {
    std::istringstream is("num_custom_presets=99\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.num_custom_presets <= Config::MAX_CUSTOM_PRESETS);
}

TEST_CASE("Config custom preset values clamped", "[config]") {
    std::istringstream is("num_custom_presets=1\ncustom_preset0=100000\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.custom_preset_secs[0] == Config::TIMER_MAX_SECS);
}

TEST_CASE("Config custom preset index out of range ignored", "[config]") {
    std::istringstream is("num_custom_presets=1\ncustom_preset9=300\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.custom_preset_secs[0] == 0);
}

TEST_CASE("Config custom presets defaults to zero", "[config]") {
    Config c;
    REQUIRE(c.num_custom_presets == 0);
    for (int i = 0; i < Config::MAX_CUSTOM_PRESETS; ++i)
        REQUIRE(c.custom_preset_secs[i] == 0);
}

TEST_CASE("Config analog style defaults", "[config]") {
    AnalogClockStyle s;
    REQUIRE(s.hour_color == -1);
    REQUIRE(s.minute_color == -1);
    REQUIRE(s.second_color == -1);
    REQUIRE(s.face_color == -1);
    REQUIRE(s.tick_color == -1);
    REQUIRE(s.background_color == -1);
    REQUIRE(s.face_fill_color == -1);
    REQUIRE(s.face_outline_color == -1);
    REQUIRE(s.hour_label_color == -1);
    REQUIRE(s.center_dot_color == -1);
    REQUIRE(s.hour_len_pct == 60);
    REQUIRE(s.minute_len_pct == 80);
    REQUIRE(s.second_len_pct == 90);
    REQUIRE(s.hour_thickness == 4);
    REQUIRE(s.minute_thickness == 2);
    REQUIRE(s.second_thickness == 1);
    REQUIRE(s.center_dot_size == 3);
    REQUIRE(s.hour_opacity_pct == 100);
    REQUIRE(s.minute_opacity_pct == 100);
    REQUIRE(s.second_opacity_pct == 100);
    REQUIRE(s.tick_opacity_pct == 100);
    REQUIRE(s.face_opacity_pct == 100);
    REQUIRE(s.radius_pct == 100);
    REQUIRE(s.show_minute_ticks);
    REQUIRE(s.hour_labels == HourLabels::Sparse);
}

TEST_CASE("Config analog style round-trip", "[config]") {
    Config orig;
    orig.analog_style.hour_color = 0xFF0000;
    orig.analog_style.minute_color = 0x00FF00;
    orig.analog_style.second_color = 0x0000FF;
    orig.analog_style.face_color = 0xAAAAAA;
    orig.analog_style.tick_color = 0x555555;
    orig.analog_style.background_color = 0x101010;
    orig.analog_style.face_fill_color = 0x202020;
    orig.analog_style.face_outline_color = 0x303030;
    orig.analog_style.hour_label_color = 0x404040;
    orig.analog_style.center_dot_color = 0x505050;
    orig.analog_style.hour_len_pct = 55;
    orig.analog_style.minute_len_pct = 85;
    orig.analog_style.second_len_pct = 95;
    orig.analog_style.hour_thickness = 5;
    orig.analog_style.minute_thickness = 3;
    orig.analog_style.second_thickness = 2;
    orig.analog_style.hour_tick_pct = 15;
    orig.analog_style.minute_tick_pct = 7;
    orig.analog_style.center_dot_size = 6;
    orig.analog_style.hour_opacity_pct = 90;
    orig.analog_style.minute_opacity_pct = 80;
    orig.analog_style.second_opacity_pct = 70;
    orig.analog_style.tick_opacity_pct = 60;
    orig.analog_style.face_opacity_pct = 50;
    orig.analog_style.radius_pct = 75;
    orig.analog_style.show_minute_ticks = false;
    orig.analog_style.hour_labels = HourLabels::Full;

    std::ostringstream os;
    config_write(orig, os);

    Config back;
    std::istringstream is(os.str());
    config_read(back, is);

    REQUIRE(back.analog_style.hour_color == 0xFF0000);
    REQUIRE(back.analog_style.minute_color == 0x00FF00);
    REQUIRE(back.analog_style.second_color == 0x0000FF);
    REQUIRE(back.analog_style.face_color == 0xAAAAAA);
    REQUIRE(back.analog_style.tick_color == 0x555555);
    REQUIRE(back.analog_style.background_color == 0x101010);
    REQUIRE(back.analog_style.face_fill_color == 0x202020);
    REQUIRE(back.analog_style.face_outline_color == 0x303030);
    REQUIRE(back.analog_style.hour_label_color == 0x404040);
    REQUIRE(back.analog_style.center_dot_color == 0x505050);
    REQUIRE(back.analog_style.hour_len_pct == 55);
    REQUIRE(back.analog_style.minute_len_pct == 85);
    REQUIRE(back.analog_style.second_len_pct == 95);
    REQUIRE(back.analog_style.hour_thickness == 5);
    REQUIRE(back.analog_style.minute_thickness == 3);
    REQUIRE(back.analog_style.second_thickness == 2);
    REQUIRE(back.analog_style.hour_tick_pct == 15);
    REQUIRE(back.analog_style.minute_tick_pct == 7);
    REQUIRE(back.analog_style.center_dot_size == 6);
    REQUIRE(back.analog_style.hour_opacity_pct == 90);
    REQUIRE(back.analog_style.minute_opacity_pct == 80);
    REQUIRE(back.analog_style.second_opacity_pct == 70);
    REQUIRE(back.analog_style.tick_opacity_pct == 60);
    REQUIRE(back.analog_style.face_opacity_pct == 50);
    REQUIRE(back.analog_style.radius_pct == 75);
    REQUIRE_FALSE(back.analog_style.show_minute_ticks);
    REQUIRE(back.analog_style.hour_labels == HourLabels::Full);
}

TEST_CASE("Config analog style not written when defaults", "[config]") {
    Config orig;
    std::ostringstream os;
    config_write(orig, os);
    REQUIRE(os.str().find("analog_") == std::string::npos);
}

TEST_CASE("Config analog style values clamped", "[config]") {
    std::istringstream is("analog_hour_len=150\nanalog_minute_thick=10\nanalog_hour_labels=5\nanalog_radius=20\nanalog_face_opacity=-5\nanalog_center_dot_size=99\n");
    Config c;
    config_read(c, is);
    REQUIRE(c.analog_style.hour_len_pct == 100);
    REQUIRE(c.analog_style.minute_thickness == 4);
    REQUIRE(c.analog_style.hour_labels == HourLabels::Full);
    REQUIRE(c.analog_style.radius_pct == 50);
    REQUIRE(c.analog_style.face_opacity_pct == 0);
    REQUIRE(c.analog_style.center_dot_size == 10);
}
