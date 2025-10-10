#pragma once

#include <cstdint>
#include <regex>
#include <string>

const std::regex int_re(R"(^[+-]?\d+$)");
const std::regex float_re(R"(^[+-]?\d*\.\d+([eE][+-]?\d+)?$)");
const std::regex sci_re(R"(^[+-]?\d+([eE][+-]?\d+)$)");

struct DBEntry {
    std::string value;
    int64_t expiration;
};

enum class Operation { GET, SET, SETEX, DELETE, PERSIST, EXPIRE };

struct Request {
    Operation action;
    std::string key;
    std::string value;
    int64_t expiration;
};

const std::map<std::string, Operation> action_mapping = {{"get", Operation::GET},
                                                         {"set", Operation::SET},
                                                         {"setex", Operation::SETEX},
                                                         {"persist", Operation::PERSIST},
                                                         {"expire", Operation::EXPIRE}};