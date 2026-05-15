#pragma once
#include <istream>
#include <ostream>
#include "config.hpp"

bool config_write(const Config& c, std::ostream& f);
bool config_read(Config& c, std::istream& f);
