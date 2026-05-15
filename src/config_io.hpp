#pragma once
#include <filesystem>
#include <windows.h>
#include "wndstate.hpp"

std::filesystem::path config_path();
void save_config(HWND hwnd, const WndState& s);
void load_config(HWND hwnd, WndState& s);
