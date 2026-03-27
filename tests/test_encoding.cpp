#include <catch2/catch_test_macros.hpp>
#include <string>
import encoding;

TEST_CASE("wide_to_utf8 ASCII", "[encoding]") {
    REQUIRE(wide_to_utf8(L"hello") == "hello");
    REQUIRE(wide_to_utf8(L"") == "");
    REQUIRE(wide_to_utf8(L"ABC 123") == "ABC 123");
}

TEST_CASE("wide_to_utf8 multibyte", "[encoding]") {
    // U+4E2D U+6587 — "中文" — 3-byte sequences in UTF-8
    std::wstring chinese = {L'\u4E2D', L'\u6587'};
    std::string expected = "\xE4\xB8\xAD\xE6\x96\x87";
    REQUIRE(wide_to_utf8(chinese) == expected);

    // U+00E9 — "é" — 2-byte sequence in UTF-8
    std::wstring accented = {L'\u00E9'};
    REQUIRE(wide_to_utf8(accented) == "\xC3\xA9");
}

TEST_CASE("utf8_to_wide ASCII", "[encoding]") {
    REQUIRE(utf8_to_wide("hello") == L"hello");
    REQUIRE(utf8_to_wide("") == L"");
    REQUIRE(utf8_to_wide("ABC 123") == L"ABC 123");
}

TEST_CASE("utf8_to_wide multibyte", "[encoding]") {
    std::wstring expected_chinese = {L'\u4E2D', L'\u6587'};
    REQUIRE(utf8_to_wide("\xE4\xB8\xAD\xE6\x96\x87") == expected_chinese);

    std::wstring expected_e = {L'\u00E9'};
    REQUIRE(utf8_to_wide("\xC3\xA9") == expected_e);
}

TEST_CASE("wide_to_utf8 / utf8_to_wide round-trip", "[encoding]") {
    std::wstring inputs[] = {
        L"hello",
        L"",
        {L'\u4E2D', L'\u6587'},       // 中文
        {L'\u00E9', L'\u00FC'},        // éü
        L"Timer label",
    };
    for (const auto& w : inputs) {
        REQUIRE(utf8_to_wide(wide_to_utf8(w)) == w);
    }
}
