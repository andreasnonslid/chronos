#pragma once
#include <filesystem>
#include "app.hpp"

std::filesystem::path config_path();
void save_config(App& app, const std::filesystem::path& path);
bool load_config(App& app, const std::filesystem::path& path);
