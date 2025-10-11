#include "utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <sstream>

#include "exceptions.h"

std::string to_lower(const std::string& str) {
    std::string out(str);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

Operation string_to_op(const std::string& s) {
    auto it = action_mapping.find(to_lower(s));
    if (it == action_mapping.end()) {
        throw InvalidCommandException("Unknown command");
    }
    return it->second;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(s);

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

uint64_t seconds_since_epoch() {
    auto now = std::chrono::system_clock::now();
    return duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}