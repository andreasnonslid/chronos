#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>
import config;

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
