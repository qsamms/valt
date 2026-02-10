#pragma once

#include <types/types.h>

#include <string>
#include <vector>

namespace utils {

std::string to_lower(const std::string& str);

Operation to_operation(const std::string& str);

std::vector<std::string> split(const std::string& s, char delimiter);

uint64_t seconds_since_epoch();

void set_nonblocking(int client_fd);

std::string escape_string(const std::string& s);

}  // namespace utils
