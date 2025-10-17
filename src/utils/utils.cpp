#include "utils.h"

#include <fcntl.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>

#include "exceptions.h"
#include "types.h"

using RuntimeError = std::runtime_error;

std::string to_lower(const std::string& str) {
    std::string out(str);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

Operation string_to_op(const std::string& s) {
    auto it = op_mapping.find(to_lower(s));
    if (it == op_mapping.end()) {
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

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw RuntimeError("set nonblocking failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) throw RuntimeError("set nonblocking failed");
}

std::string escape_string(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '\n':
                result += "\\n";
                break;
            case '\t':
                result += "\\t";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\"':
                result += "\\\"";
                break;
            default:
                result += c;
                break;
        }
    }
    return result;
}
