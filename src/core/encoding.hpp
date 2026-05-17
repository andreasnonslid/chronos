#pragma once
#include <string>

std::string  wide_to_utf8(const std::wstring& w);
std::wstring utf8_to_wide(const std::string& s);
