#pragma once

#include <string>
#include <vector>

#include "types.h"

std::string to_lower(const std::string& str);
Operation string_to_op(const std::string& str);
std::vector<std::string> split(const std::string& s, char delimiter);
uint64_t seconds_since_epoch();